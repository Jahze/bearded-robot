export void main()
{
	vec4 directionToLight = g_light0_position - g_world_position;
	float lightDistance = length(directionToLight);

	directionToLight = normalize(directionToLight);

	float dp = dot3(g_world_normal, directionToLight);
	float clamped = clamp(dp, 0.0, dp);

	g_colour[0] = 0.0;
	g_colour[1] = 0.0;

	if (clamped > 0.75)
	{
		g_colour[2] = 1.0;
	}
	else if (clamped > 0.5)
	{
		g_colour[2] = 0.6;
	}
	else if (clamped > 0.25)
	{
		g_colour[2] = 0.4;
	}
	else
	{
		g_colour[2] = 0.2;
	}

	return;
}