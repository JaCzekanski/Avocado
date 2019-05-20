in vec2 fragTexcoord;
out vec4 outColor;
uniform sampler2D vram;

void main() {
    outColor = vec4(texture(vram, fragTexcoord).rgb, 1.0);
}