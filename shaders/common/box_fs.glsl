#version 330 core

uniform vec4 color;
uniform bool renderBright;

out vec4 FragColor;
out vec4 BrightColor;

void main()
{
    FragColor = color;
    if (renderBright) {
        float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0) {
            BrightColor = vec4(FragColor.rgb, 1.0);
        }
	    else {
	    	BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
}