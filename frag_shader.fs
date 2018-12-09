#version 330 core
out vec4 frag_color;

in vec2 texc;
in vec3 norml;
in vec3 frag_pos;

struct Light 
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform sampler2D current_tex;
uniform sampler2D select_tex;
uniform int selected;
uniform float time;
uniform float shininess;
uniform Light light;
uniform vec3 view_pos;

void main()
{
	vec3 color;
	if(selected == 0)
	{
		color = texture(current_tex, texc).rgb;
	}
	else
	{
		color = mix(texture(current_tex, texc), texture(select_tex, texc), time).rgb;
	}
	vec3 ambient = light.ambient * color;
	
	vec3 norm = normalize(norml);
	vec3 light_dir = normalize(light.position - frag_pos);
	float diff = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = light.diffuse * diff * color;
	
	//vec3 view_dir = normalize(view_pos - frag_pos);
	//vec3 reflect_dir = reflect(-light_dir, norm);
	//float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	//vec3 specular = light.specular * spec * color;
	
	frag_color = vec4(ambient + diffuse, 1.0);
}