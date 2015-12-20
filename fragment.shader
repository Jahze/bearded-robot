export void main()
{
	vec4 directionToLight = g_light0_position - g_world_position;
	float lightDistance = length(directionToLight);

	directionToLight = normalize(directionToLight);

	vec4 ambient;
	ambient[0] = 0.2;
	ambient[1] = 0.2;
	ambient[2] = 0.2;

	vec4 diffuse;
	diffuse[0] = 0.8;
	diffuse[1] = 0.8;
	diffuse[2] = 0.8;

	float dp = dot3(g_world_normal, directionToLight);
	float clamped = clamp(dp, 0.0, dp);

	vec4 colour = ambient + (diffuse * clamped);

	g_colour[0] = clamp(colour[0], 0.0, 1.0);
	g_colour[1] = clamp(colour[1], 0.0, 1.0);
	g_colour[2] = clamp(colour[2], 0.0, 1.0);

	return;
}