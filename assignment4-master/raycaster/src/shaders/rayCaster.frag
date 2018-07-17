// Fragment shader
#version 150

in vec2 v_texcoord;

out vec4 frag_color;

uniform float u_step_size;
uniform int u_mode;

uniform vec3 u_tf1_color;
uniform vec3 u_tf2_color;
uniform vec3 u_tf3_color;
uniform vec3 u_tf4_color;

uniform float u_tf1_intensity;
uniform float u_tf2_intensity;
uniform float u_tf3_intensity;
uniform float u_tf4_intensity;

uniform float u_tf1_alpha;
uniform float u_tf2_alpha;

uniform float u_sample_rate;
uniform float u_cor;
uniform int u_cor_enable;

uniform sampler3D u_volumeTexture;
uniform sampler2D u_backFaceTexture;
uniform sampler2D u_frontFaceTexture;

 // Color lookup table.
vec4 lut(float i) {
	vec4 grayscale = vec4(i*u_tf1_alpha);

	if(i >= u_tf1_intensity) {
      	grayscale *= vec4(u_tf1_color * u_tf2_alpha, 1.0);
    }
    else if(i >= u_tf2_intensity) {
       	grayscale *= vec4(u_tf2_color * u_tf2_alpha, 1.0);
    }
    else if(i >= u_tf3_intensity) {
       	grayscale *= vec4(u_tf3_color * u_tf2_alpha, 1.0);
    }
    else if (i >= u_tf4_intensity) {
       	grayscale *= vec4(u_tf4_color * u_tf2_alpha, 1.0);
    }
    
    return grayscale; //bad name as it is likely not grayscale after if-block
}

void main()
{
	// Get texture from uniforms as starting and ending coordinates.
	vec3 ray_start = texture(u_frontFaceTexture, v_texcoord).xyz;
	vec3 ray_end = texture(u_backFaceTexture, v_texcoord).xyz;
	
	// Remove ray casting the background
	if(ray_start == ray_end) {
		discard;
	}

	// Compute ray parameters
	vec3 ray = ray_end - ray_start;
	float ray_length = length(ray);

	// Convert passed uniform stepsize into step
	vec3 ray_delta = u_step_size * ray / ray_length;
	//vec3 ray_delta = normalize(ray) * u_step_size;

	float ray_delta_length = length(ray_delta);

	// Initialize final color and voxel position
    vec3 voxel_coord = ray_start;
    vec4 color = vec4(0);
    
    // Initialize samples
    vec4 color_sample; // src color
    float alpha_sample; // src alpha
    float intensity; // sampled intensity
    float intensity_out; // dest blending alpha

    vec4 color_out = vec4(0); //final output
    float alpha_out = 0.0; //final alpha

    // Front to back Alpha blending implementation
    if(u_mode == 0)
    {
        while (color_out.a < 1.0 && ray_length >= 0) {
        	intensity = texture(u_volumeTexture, voxel_coord).x;
        	color_sample = lut(intensity);

        	// Interpolation
        	if(color_sample.a > 0.0) {
        		color_sample.a = 1.0 - pow(1.0 - color_sample.a, u_step_size * u_sample_rate);
        		color_out.rgb += (1.0 - color_out.a) * color_sample.rgb * color_sample.a;
        		color_out.a += (1.0 - color_out.a) * color_sample.a;
        	}
        	voxel_coord += ray_delta;
        	ray_length -= u_step_size;
        }

        color_out.a = 1.0;	
        color = color_out;
    }

    // Maximum Intensity Projection
    else {
    	float max_sample = 0.0;

    	while (ray_length > 0) { 
    		float sample = texture(u_volumeTexture, voxel_coord).x;
    		if(sample > max_sample) {
    			max_sample = sample;
    		}
    		voxel_coord += ray_delta;
    		ray_length -= u_step_size;
    	}

    	color = vec4(max_sample, max_sample, max_sample, 1);
    }

    // remove cube border and model artifacts produced from texture background
    // ideally black is zero but needs some arbitrary threshold
    if(u_cor_enable == 1) {
    	if(color.x <= u_cor && color.y <= u_cor && color.z <= u_cor) {
       		discard;
    	}	
    }

    frag_color = color;
}