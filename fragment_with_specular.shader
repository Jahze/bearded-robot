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
	diffuse[0] = 0.7;
	diffuse[1] = 0.7;
	diffuse[2] = 0.7;

	vec4 specular;
	specular[0] = 1.0;
	specular[1] = 1.0;
	specular[2] = 1.0;

	float dp = dot3(g_world_normal, directionToLight);
	float clamped = clamp(dp, 0.0, dp);

	float specular_factor = 0.0;

	if (clamped > 0.0)
	{
		float reflect_dp = 2.0 * dot3(-directionToLight, g_world_normal);
		vec4 reflected = normalize(-directionToLight - g_world_normal * reflect_dp);

		vec4 view_direction = normalize(-g_world_position);

		float angle = dot3(reflected, view_direction);
		float clamped_angle = clamp(angle, 0.0, angle);

		specular_factor = clamped_angle * clamped_angle * clamped_angle * clamped_angle;
		specular_factor *= specular_factor;
		specular_factor *= specular_factor;
	}

	vec4 colour = ambient + (diffuse * clamped) + (specular * specular_factor);

	g_colour[0] = clamp(colour[0], 0.0, 1.0);
	g_colour[1] = clamp(colour[1], 0.0, 1.0);
	g_colour[2] = clamp(colour[2], 0.0, 1.0);

	return;
}