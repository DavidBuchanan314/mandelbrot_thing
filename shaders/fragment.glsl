#version 420

#define UNROLL_LOOP 1
#define USE_DOUBLES 1

#if USE_DOUBLES
#define VEC2 dvec2
#define VEC3 dvec3
#define FLOAT double
#define DOT(z) (z.x*z.x+z.y*z.y)
#else
#define VEC2 vec2
#define VEC3 vec3
#define FLOAT float
#define DOT(z) dot(z,z)
#endif

#define CPLX_SQUARE(z) VEC2( z.x*z.x - z.y*z.y, 2.0*z.x*z.y )

uniform float time;
uniform FLOAT scale;
uniform float angle;
uniform VEC2 midpoint;
uniform VEC2 resolution;
uniform vec2 bounds;

out vec4 outputColor;

void main() {
	float aa = min(sqrt(1.0/bounds.y), 4.0); // number of sub-sample steps per axis
	float st = 1.0/aa; // sub-pixel step size
	float imax = 160.0 + (1.0/bounds.y)*40.0; // increase iteration depth for smaller render sizes
	vec3 col = vec3(0.0);
	mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
	const float B = 2.0;
	const float PI = 3.1415926535897932384626433832795;
	
	for (float dy=0.0; dy<=0.99999; dy+=st)
	for (float dx=0.0; dx<=0.99999; dx+=st) {
		// screen position
		VEC2 p = (-resolution.xy + 2.0*(gl_FragCoord.xy+VEC2(dx, dy)))/resolution.y/2.0;

		// "world position" - position of c on the complex plane
		VEC2 c = midpoint + p*rotation*scale;

		float n = 0.0;
		VEC2 z  = VEC2(0.0);
		for (;;) {
#if UNROLL_LOOP
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			if (DOT(z) > (B*B)) { n+=5.0; break; }
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			if (DOT(z) > (B*B)) { n+=10.0; break; }
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			if (DOT(z) > (B*B)) { n+=15.0; break; }
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			z = CPLX_SQUARE(z) + c;
			if (DOT(z) > (B*B)) { n+=20.0; break; }
			
			n += 20.0;
#else
			z = CPLX_SQUARE(z) + c;
			n++;
			if (DOT(z) > (B*B)) break;
#endif
			if (n >= imax) break;
		}
		if (n >= imax) continue;
		
		float nsmooth = n - log2(log2(float(sqrt(DOT(z))))); // smooth colouring
		nsmooth = max(nsmooth, 0.0);
		//nsmooth += time*20; // trippy
		//nsmooth += log2(float(scale))*40.0; // change colours with scale
		col += 0.5 + 0.5*cos(nsmooth*0.04 + vec3(0.0, 2.0*PI/3.0, 2.0*PI/3.0*2.0));
	}

	outputColor = vec4(col/(aa*aa), 1.0);
}
