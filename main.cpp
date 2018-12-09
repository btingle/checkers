#include "includes.h"
#include "shader.h"
#include "obj_loader.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

using namespace std;

int screen_width;
int screen_height;

Shader shader_prog;
unsigned int shader_model_loc;
unsigned int shader_view_loc;
unsigned int shader_projection_loc;
unsigned int shader_texture_loc;
unsigned int shader_time_loc;
unsigned int shader_selected_loc;
unsigned int shader_viewpos_loc;

unsigned int board_tex = 0;
unsigned int select_tex = 0;
unsigned int red_piece_tex = 0;
unsigned int black_piece_tex = 0;
#define P1_BACKGROUND glm::vec3(1.f, 0.f, 0.f)
#define P2_BACKGROUND glm::vec3(0.1f, 0.1f, 0.1f)

unsigned int board_data_size = 0;
unsigned int piece_data_size = 0;

unsigned int board_VAO = 0; // VAO must be defined for each distinct buffer object, could potentially combine buffers, but don't feel like it
unsigned int board_VBO = 0;
unsigned int piece_VAO = 0;
unsigned int piece_VBO = 0;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
#define IDENTITY glm::mat4(1.f)

glm::vec3 board_cursor_mesh [9][9];
glm::vec2 transformed_cursor_mesh [9][9];

glm::vec3  select_pos;
glm::ivec2 hover_pos;
glm::ivec2 selected_piece;
unsigned char selected_value;
unsigned char checkers_board [8][8];
bool player_turn;
bool extra_move;
bool hover_lock;
bool anim_lock;
#define PLAYER_1 true
#define PLAYER_2 false
#define EMPTY_SELECT glm::ivec2(-1, -1)
#define EMPTY    0
#define P1_PIECE 1
#define P2_PIECE 2
#define P1_KING  3
#define P2_KING  4
#define DOWN  0
#define UP    1
#define OMNI  2
#define NO_MOVE 0
#define MOVE  1
#define JUMP  2

/**********************************
 *	BOARD CURSOR HOVER FUNCTIONS
 **********************************/

// the mesh that allows checking each square for cursor hovering
void gen_board_cursor_mesh()
{
	glm::vec4 transformed_vert;
	for (int y = 0; y < 9; y++)
	{
		for (int x = 0; x < 9; x++)
		{
			float xpos = -0.875f + (x * (1.75f / 8.f)); // starts from beginning of squares, ends at the end of squares
			float zpos = -0.875f + (y * (1.75f / 8.f));
			board_cursor_mesh[y][x] = glm::vec3(xpos, 0.05, zpos);
		}
	}
}

// updates the mesh to the current board view
void update_hover_mesh()
{
	glm::vec4 transformed_vert;
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			transformed_vert = projection * view * model * glm::vec4(board_cursor_mesh[y][x], 1.f);
			transformed_vert /= transformed_vert.w;
			transformed_vert = (transformed_vert + 1.f) / 2.f;
			transformed_cursor_mesh[y][x] = glm::vec2(transformed_vert);
		}
	}
}

// the sign and in_triangle functions are copy-pasted. I don't fully understand how they work, but I just need to check if a point is within a triangle
float sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
{
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

float in_triangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3)
{
	float d1, d2, d3;
	bool has_neg, has_pos;

	d1 = sign(pt, v1, v2);
	d2 = sign(pt, v2, v3);
	d3 = sign(pt, v3, v1);

has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

return !(has_neg && has_pos);
}

glm::ivec2 check_square_hover(double mouseX, double mouseY)
{
	glm::vec2 mousepos = glm::vec2((mouseX / (float)screen_width), 1 - (mouseY / (float)screen_height)); // converts mouse position coordinates to openGL window coordinates
	glm::vec2 tv1; // transformed vertices
	glm::vec2 tv2;
	glm::vec2 tv3;
	glm::vec2 tv4;
	for (int y = 0; y < 8; y++)
	{
		for (int x = y % 2; x < 8; x += 2)
		{
			tv1 = transformed_cursor_mesh[y + 0][x + 0];
			tv2 = transformed_cursor_mesh[y + 0][x + 1];
			tv3 = transformed_cursor_mesh[y + 1][x + 0];
			tv4 = transformed_cursor_mesh[y + 1][x + 1];
			if (in_triangle(mousepos, tv1, tv2, tv3) || in_triangle(mousepos, tv2, tv3, tv4))
			{
				select_pos = (board_cursor_mesh[y + 0][x + 0] + board_cursor_mesh[y + 1][x + 1]) / 2.f;
				return glm::ivec2(x, y);
			}
		}
	}
	return EMPTY_SELECT;
}

/*************************
 *	CHECKERS GAME LOGIC
 *************************/
// note: this block doesn't contain all of the checkers logic
// see the mouse_click_callback function for more

void init_game_board()
{
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			checkers_board[y][x] = EMPTY;
		}
	}
	// player 1 init "bottom" of board
	for (int y = 0; y < 3; y++)
	{
		for (int x = y % 2; x < 8; x += 2)
		{
			checkers_board[y][x] = P1_PIECE;
		}
	}
	// player 2 init "top" of board
	for (int y = 5; y < 8; y++)
	{
		for (int x = y % 2; x < 8; x += 2)
		{
			checkers_board[y][x] = P2_PIECE;
		}
	}
}

// decides if this is a legal move, then moves piece (or doesn't)
// I just realized that I've distributed half the logic of moving pieces between this function and the on-mouse-click function
int move_piece(glm::ivec2 move_loc, bool jump_only)
{
	glm::ivec2 move_mag = move_loc - selected_piece;
	unsigned char move_dest = checkers_board[move_loc.y][move_loc.x];
	int move_mod = selected_value > 2 ? OMNI : move_mag.y > 0 ? UP : DOWN; // if piece is a king, it can move anywhere
	int jump_mod = abs(move_mag.x);

	// eliminate invalid moves
	if (move_mag.x == 0 && move_mag.y == 0) return NO_MOVE;
	if (abs(move_mag.x) != abs(move_mag.y)) return NO_MOVE;
	if (jump_mod > 2) return NO_MOVE;

	if ((player_turn == PLAYER_1 && move_mod == UP) || (player_turn == PLAYER_2 && move_mod == DOWN) || move_mod == OMNI)
	{
		if (jump_mod == JUMP)
		{
			glm::ivec2 jumpover_loc = move_loc - (move_mag / 2);
			unsigned char jumped_over = checkers_board[jumpover_loc.y][jumpover_loc.x];
			// small note: all this logic is super explicit on purpose. I could make this logic a little less ugly, but would obfuscate it in the process, so I'm keeping it as is.
			// at least, that's what I'm telling myself
			if (((jumped_over == P1_PIECE || jumped_over == P1_KING) && player_turn == PLAYER_2) || ((jumped_over == P2_PIECE || jumped_over == P2_KING) && player_turn == PLAYER_1))
			{
				checkers_board[jumpover_loc.y][jumpover_loc.x] = 0;
				checkers_board[selected_piece.y][selected_piece.x] = 0;
				if ((move_loc.y == 7 && player_turn == PLAYER_1) || (move_loc.y == 0 && player_turn == PLAYER_2))
				{
					checkers_board[move_loc.y][move_loc.x] = player_turn == PLAYER_1 ? P1_KING : P2_KING;
				}
				else
				{
					checkers_board[move_loc.y][move_loc.x] = selected_value;
				}
				selected_piece = move_loc;
				selected_value = checkers_board[selected_piece.y][selected_piece.x];
				return JUMP;
			}
		}
		else if (jump_mod == MOVE && !jump_only)
		{
			checkers_board[selected_piece.y][selected_piece.x] = 0;
			if ((move_loc.y == 7 && player_turn == PLAYER_1) || (move_loc.y == 0 && player_turn == PLAYER_2))
			{
				checkers_board[move_loc.y][move_loc.x] = player_turn == PLAYER_1 ? P1_KING : P2_KING;
			}
			else
			{
				checkers_board[move_loc.y][move_loc.x] = selected_value;
			}
			selected_piece = move_loc;
			selected_value = checkers_board[selected_piece.y][selected_piece.x];
			return MOVE;
		}
	}
	return NO_MOVE;
}

// after a jump move, checks for extra moves that could be made
// kind of ugly, but it gets the job done
bool check_extra_moves(glm::ivec2 piece_loc)
{
	unsigned char piece_val = checkers_board[piece_loc.y][piece_loc.x];
	int move_mod = (piece_val == P1_KING || piece_val == P2_KING) ? OMNI : player_turn == PLAYER_1 ? UP : DOWN; // ternary nesting is fun!
	if ((move_mod == UP || move_mod == OMNI) && piece_loc.y < 6)
	{
		glm::ivec2 pmove1 = glm::ivec2(piece_loc.x + 2, piece_loc.y + 2);
		glm::ivec2 pmove2 = glm::ivec2(piece_loc.x - 2, piece_loc.y + 2);
		unsigned char dest_val1 = EMPTY, inbt_val1 = EMPTY, dest_val2 = EMPTY, inbt_val2 = EMPTY;
		if (piece_loc.x < 6)
		{
			dest_val1 = checkers_board[pmove1.y][pmove1.x];
			inbt_val1 = checkers_board[pmove1.y - 1][pmove1.x - 1];
		}
		if (piece_loc.x > 1)
		{
			dest_val2 = checkers_board[pmove2.y][pmove2.x];
			inbt_val2 = checkers_board[pmove2.y - 1][pmove2.x + 1];
		}
		if (dest_val1 == EMPTY)
		{
			return (((inbt_val1 == P1_PIECE || inbt_val1 == P1_KING) && player_turn == PLAYER_2) || ((inbt_val1 == P2_PIECE || inbt_val1 == P2_KING) && player_turn == PLAYER_1));
		}
		if (dest_val2 == EMPTY)
		{
			return (((inbt_val2 == P1_PIECE || inbt_val2 == P1_KING) && player_turn == PLAYER_2) || ((inbt_val2 == P2_PIECE || inbt_val2 == P2_KING) && player_turn == PLAYER_1));
		}
	}
	if ((move_mod == DOWN || move_mod == OMNI) && piece_loc.y > 1)
	{
		glm::ivec2 pmove1 = glm::ivec2(piece_loc.x + 2, piece_loc.y - 2);
		glm::ivec2 pmove2 = glm::ivec2(piece_loc.x - 2, piece_loc.y - 2);
		unsigned char dest_val1 = EMPTY, inbt_val1 = EMPTY, dest_val2 = EMPTY, inbt_val2 = EMPTY;
		if (piece_loc.x < 6)
		{
			dest_val1 = checkers_board[pmove1.y][pmove1.x];
			inbt_val1 = checkers_board[pmove1.y + 1][pmove1.x - 1];
		}
		if (piece_loc.x > 1)
		{
			dest_val2 = checkers_board[pmove2.y][pmove2.x];
			inbt_val2 = checkers_board[pmove2.y + 1][pmove2.x + 1];
		}
		if (dest_val1 == EMPTY)
		{
			return (((inbt_val1 == P1_PIECE || inbt_val1 == P1_KING) && player_turn == PLAYER_2) || ((inbt_val1 == P2_PIECE || inbt_val1 == P2_KING) && player_turn == PLAYER_1));
		}
		if (dest_val2 == EMPTY)
		{
			return (((inbt_val2 == P1_PIECE || inbt_val2 == P1_KING) && player_turn == PLAYER_2) || ((inbt_val2 == P2_PIECE || inbt_val2 == P2_KING) && player_turn == PLAYER_1));
		}
	}
	return false;
}

/*************************
 *	GAME DRAW FUNCTIONS
 *************************/

void draw_game_board()
{
	model = IDENTITY;
	shader_prog.setMat4(shader_model_loc, model);
	shader_prog.setInt(shader_texture_loc, 0);
	glBindVertexArray(board_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, board_VBO);
	glDrawArrays(GL_TRIANGLES, 0, board_data_size);
}

void draw_select_square()
{
	model = glm::translate(IDENTITY, select_pos);
	model = glm::scale(model, glm::vec3(0.109375f, 0.109375f, 0.109375f));
	shader_prog.setMat4(shader_model_loc, model);
	shader_prog.setInt(shader_texture_loc, 1);
	glDrawArrays(GL_TRIANGLES, 0, board_data_size);
}

void draw_pieces()
{
	glBindVertexArray(piece_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, piece_VBO);
	shader_prog.setInt(shader_texture_loc, 2);

	glm::vec3 piece_pos;
	unsigned char piece;
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			piece = checkers_board[y][x];
			if (x == selected_piece.x && y == selected_piece.y) shader_prog.setInt(shader_selected_loc, 1);
			if (piece == P1_PIECE || piece == P1_KING) shader_prog.setInt(shader_texture_loc, 2); // p1 - red
			if (piece == P2_PIECE || piece == P2_KING) shader_prog.setInt(shader_texture_loc, 3); // p2 - black
			if (piece == P1_PIECE || piece == P2_PIECE)
			{
				piece_pos = (board_cursor_mesh[y + 0][x + 0] + board_cursor_mesh[y + 1][x + 1]) / 2.f;
				model = glm::translate(IDENTITY, piece_pos);
				shader_prog.setMat4(shader_model_loc, model);
				glDrawArrays(GL_TRIANGLES, 0, piece_data_size);
			}
			if (piece == P1_KING || piece == P2_KING)
			{
				piece_pos = (board_cursor_mesh[y + 0][x + 0] + board_cursor_mesh[y + 1][x + 1]) / 2.f;
				model = glm::translate(IDENTITY, piece_pos);
				shader_prog.setMat4(shader_model_loc, model);
				glDrawArrays(GL_TRIANGLES, 0, piece_data_size);
				model = glm::translate(model, glm::vec3(0.f, 0.02, 0.f));
				shader_prog.setMat4(shader_model_loc, model);
				glDrawArrays(GL_TRIANGLES, 0, piece_data_size);
			}
			if (x == selected_piece.x && y == selected_piece.y) shader_prog.setInt(shader_selected_loc, 0);
		}
	}
	model = IDENTITY;
}

/*************************************
 *	MAIN, INIT, AND WINDOW CALLBACK FUNCTIONS
 *************************************/

void reshape_wind(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	projection = glm::perspective((float)glm::radians(45.f), (float)width / (float)height, 0.1f, 100.0f);
}

// clicks change the state of the checkers game
// so this function contains a lot of checkers logic
void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && !anim_lock)
	{
		if (hover_pos.x != -1)
		{
			unsigned char hover_val = checkers_board[hover_pos.y][hover_pos.x];
			bool valid_select = ((hover_val == P1_PIECE || hover_val == P1_KING) && player_turn == PLAYER_1) || ((hover_val == P2_PIECE || hover_val == P2_KING) && player_turn == PLAYER_2);
			if (selected_value == EMPTY)
			{
				if (valid_select)
				{
					selected_piece = hover_pos;
					selected_value = checkers_board[hover_pos.y][hover_pos.x];
				}
				else
				{
					selected_piece = EMPTY_SELECT;
					selected_value = EMPTY;
				}
			}
			else if (hover_val != EMPTY && !extra_move)
			{
				if (valid_select)
				{
					selected_piece = hover_pos;
					selected_value = hover_val;
				}
				else
				{
					selected_piece = EMPTY_SELECT;
					selected_value = EMPTY;
				}
			}
			else if (!valid_select)
			{
				int move = move_piece(hover_pos, extra_move);
				if (move == JUMP)
				{
					extra_move = check_extra_moves(hover_pos); // extra_move locks selection of pieces
					if (!extra_move) // if there are no extra moves to make, switch to other player's turn
					{
						player_turn = !player_turn;
						selected_piece = EMPTY_SELECT;
						selected_value = EMPTY;
						anim_lock = true;
						if (player_turn == PLAYER_1)
						{
							glClearColor(P1_BACKGROUND.x, P1_BACKGROUND.y, P1_BACKGROUND.z, 1.f);
						}
						else
						{
							glClearColor(P2_BACKGROUND.x, P2_BACKGROUND.y, P2_BACKGROUND.z, 1.f);
						}
					}
				}
				else if(move == MOVE)
				{
					player_turn = !player_turn;
					selected_piece = EMPTY_SELECT;
					selected_value = EMPTY;
					anim_lock = true;
					if (player_turn == PLAYER_1)
					{
						glClearColor(P1_BACKGROUND.x, P1_BACKGROUND.y, P1_BACKGROUND.z, 1.f);
					}
					else
					{
						glClearColor(P2_BACKGROUND.x, P2_BACKGROUND.y, P2_BACKGROUND.z, 1.f);
					}
				}
				else if(move == NO_MOVE)
				{
					selected_piece = EMPTY_SELECT;
					selected_value = EMPTY;
				}
			}
		}
		else if (!extra_move)
		{
			selected_piece = hover_pos;
			selected_value = EMPTY;
		}
		glfwSetTime(0);
		cout << hover_pos.x << " " << hover_pos.y << endl;
	}
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!hover_lock)
	{
		hover_pos = check_square_hover(xpos, ypos);
	}
}

bool init()
{
	// openGL init stuff

	// texture loading
	const char * tex_filenames[] = { "model_files/wood_surface_tex.png", "model_files/tile_select_tex.png", "model_files/checkers_red_texture.jpg", "model_files/checkers_black_texture.png" };
	unsigned int * tex_ids[] = { &board_tex, &select_tex, &red_piece_tex, &black_piece_tex };

	for (int i = 0; i < 4; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glGenTextures(1, tex_ids[i]);
		glBindTexture(GL_TEXTURE_2D, *tex_ids[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		int tex_w, tex_h, nr_channels;
		unsigned char * tex_data = stbi_load(tex_filenames[i], &tex_w, &tex_h, &nr_channels, 0);
		if (tex_data)
		{
			if (nr_channels == 3)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			else if (nr_channels == 4)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			else
			{
				cout << "invalid number of channels";
				return false;
			}
		}
		else
		{
			cout << "failed to load texture data";
			return false;
		}
		stbi_image_free(tex_data);
	}

	// initialize VAO

	// model vertex data loading
	
	vector<float>  obj_buffer;
	if (load_obj(obj_buffer, "model_files/checkers_board.obj"))
	{
		glGenBuffers(1, &board_VBO);
		glGenVertexArrays(1, &board_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, board_VBO);
		glBufferData(GL_ARRAY_BUFFER, obj_buffer.size() * sizeof(float), &(obj_buffer[0]), GL_STATIC_DRAW);
		board_data_size = obj_buffer.size() / 8;
		glBindVertexArray(board_VAO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // position attrib
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal attrib
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // texture attrib
		glEnableVertexAttribArray(2);
		obj_buffer.clear();
	}
	else
	{
		cout << "failed to load : model_files/checkers_board.obj";
		return false;
	}

	if (load_obj(obj_buffer, "model_files/checkers_piece.obj"))
	{
		glGenBuffers(1, &piece_VBO);
		glGenVertexArrays(1, &piece_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, piece_VBO);
		glBufferData(GL_ARRAY_BUFFER, obj_buffer.size() * sizeof(float), &(obj_buffer[0]), GL_STATIC_DRAW);
		piece_data_size = obj_buffer.size() / 8;
		glBindVertexArray(piece_VAO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // position attrib
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal attrib
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // texture attrib
		glEnableVertexAttribArray(2);
		obj_buffer.clear();
	}
	else
	{
		cout << "failed to load : model_files / checkers_piece.obj";
		return false;
	}

	// shader init
	model = IDENTITY;
	view = glm::lookAt(glm::vec3(0.f, 2.f, -2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	projection = glm::perspective((float)glm::radians(45.f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

	shader_prog = Shader("shaders/vert_shader.vs", "shaders/frag_shader.fs");
	shader_prog.use();

	shader_model_loc = shader_prog.getUniformID("model");
	shader_view_loc = shader_prog.getUniformID("view");
	shader_projection_loc = shader_prog.getUniformID("projection");
	shader_texture_loc = shader_prog.getUniformID("current_tex");
	shader_time_loc = shader_prog.getUniformID("time");
	shader_selected_loc = shader_prog.getUniformID("selected");
	shader_viewpos_loc = shader_prog.getUniformID("view_pos");

	shader_prog.setMat4(shader_projection_loc, projection);
	shader_prog.setMat4(shader_view_loc, view);
	shader_prog.setMat4(shader_model_loc, model);
	shader_prog.setInt(shader_texture_loc, 0);
	shader_prog.setInt(shader_selected_loc, 0);
	shader_prog.setVec3(shader_viewpos_loc, glm::vec3(0.f, 2.f, -2.f));

	glm::vec3 light_color(1.f, 1.f, 1.f);
	shader_prog.setVec3("light.ambient" , light_color * 0.2f);
	shader_prog.setVec3("light.diffuse" , light_color * 0.8f);
	shader_prog.setVec3("light.specular", light_color * 0.f);
	shader_prog.setVec3("light.position", glm::vec3(0.f, 3.f, 0.f));
	shader_prog.setFloat("shininess", 16.f);
	shader_prog.setInt("select_tex", 1);

	// misc openGL init
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(P1_BACKGROUND.x, P1_BACKGROUND.y, P1_BACKGROUND.z, 1.f);

	// game init
	hover_lock = false;
	player_turn = PLAYER_1;
	selected_piece = EMPTY_SELECT;
	selected_value = EMPTY;
	init_game_board();
	gen_board_cursor_mesh();
	update_hover_mesh();
	screen_width = SCREEN_WIDTH;
	screen_height = SCREEN_HEIGHT;

	return true;
}

int main()
{
	// super boilerplate stuff
	GLFWwindow* window;

	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Checkers", NULL, NULL);
	if (!window)
	{
		cout << "Failed to open window";
		char wait;
		cin >> wait;
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, reshape_wind);
	glfwSetMouseButtonCallback(window, mouse_click_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD";
		char wait;
		cin >> wait;
		return -1;
	}
	// end super boilerplate stuff

	if (!init())
	{
		// there should have been some other, more descriptive error message before this one
		char wait;
		cin >> wait;
		return -1;
	}

	float angle = 0.f;
	float time;

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		time = (cos(glfwGetTime()*2.5) + 1.5f) / 3.f;
		shader_prog.setFloat(shader_time_loc, time);

		draw_game_board();
		draw_select_square();
		draw_pieces();

		// rotate board on player turn switch
		if (anim_lock)
		{
			glm::vec3 view_pos = glm::vec3(sin(angle) * 2, 2.f, -cos(angle) * 2);
			view = glm::lookAt(view_pos, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
			shader_prog.setMat4(shader_view_loc, view);
			shader_prog.setVec3(shader_viewpos_loc, view_pos);
			angle += sqrt(abs(sin(angle))) * 0.008f + 0.0005f;
			update_hover_mesh();
			if (player_turn == PLAYER_2 && angle > M_PI)
			{
				anim_lock = false;
				angle = M_PI;
			}
			else if (player_turn == PLAYER_1 && angle > (2*M_PI))
			{
				anim_lock = false;
				angle = 0.f;
			}
		}

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &board_VAO);
	glDeleteVertexArrays(1, &piece_VAO);
	glDeleteBuffers(1, &board_VBO);
	glDeleteBuffers(1, &piece_VBO);

	glfwTerminate();

	return 0;
}