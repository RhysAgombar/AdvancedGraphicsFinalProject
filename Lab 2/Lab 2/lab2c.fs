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
vec4 khaki = vec4(0.941, 0.902, 0.549, 1.0);
vec4 blue = vec4(0.000, 0.000, 0.804, 1.0);
vec4 lgreen = vec4(0.196, 0.804, 0.196, 1.0);
vec4 dgreen = vec4(0.00, 0.392, 0.000, 1.0);
vec4 stone = vec4(0.439, 0.502, 0.565, 1.0);
vec4 snow = vec4(1.000, 0.980, 0.980, 1.0);

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
	
	float dist = 0;
	float dist2 = 0;

	if (position.y >= 9.0){
		c = snow;

	}

	if ( 7.0 <= position.y  && position.y < 9.0){
		dist = 1 - ((position.y - 7.0) - 1)/(2.0 - 1.0);
		dist2 = 1 - dist;

		if (dist >= 0.99){
			c = stone;
		} else {
			c = stone * dist + snow * dist2;
		}
	}

	if ( 4.0 <= position.y && position.y < 7.0){
		dist = 1 - ((position.y - 4.0) - 1.5)/(3.0 - 1.5);
		dist2 = 1 - dist;

		if (dist >= 0.99){
			c = dgreen;
		} else {
			c = dgreen * dist + stone * dist2;
		}

	}

	if ( 1.0 <= position.y && position.y < 4.0){
		dist = 1 - ((position.y - 1.0) - 1.5)/(3.0 - 1.5);
		dist2 = 1 - dist;

		if (dist >= 0.99) {
			c = lgreen;
		} else {
			c = lgreen * dist + dgreen * dist2;
		}		

	}

	if ( 0.01 <= position.y && position.y < 1.0){
		dist = 1 - ((position.y - 0.0) - 0.3)/(1.0 - 0.3);
		dist2 = 1 - dist;

		if (dist >= 0.99) {
			c = khaki;
		} else {
			c = khaki * dist + lgreen * dist2;
		}

	}

	if ( position.y < 0.01){
		c = blue;
	}

	gl_FragColor = min(material.x * c + material.y * diffuse * c + material.z * white * specular, vec4(1.0));
	gl_FragColor.a = c.a;


}