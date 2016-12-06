/*
 *  Simple fragment sharder for Lab 2
 */

#version 330 core

in vec3 normal;
in vec3 position;
uniform vec4 colour;
uniform vec3 Eye;
uniform vec3 light;
uniform vec4 material;

vec4 c;

void main() {

	vec4 white = vec4(1.0,1.0,1.0,1.0);
	float diffuse;
	vec3 L = normalize(light);
	vec3 R = normalize(reflect(-L,normal));
	vec3 N;
	vec3 H = normalize(L + (Eye - position));
	float specular;

	N = normalize(normal);
	diffuse = dot(N,L);
	if (diffuse < 0.0){
		diffuse = 0.0;
		specular = 0.0;
	} else {
		specular =  pow(max(0.0, dot(N,R)),material.w);
	}

	c = colour;
	
	gl_FragColor = min(material.x * c + material.y * diffuse * c + material.z * white * specular, vec4(1.0));
	gl_FragColor.a = c.a;

}