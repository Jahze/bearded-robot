export void main()
{
	vec4 position = g_position + g_normal * 0.01;

	g_projected_position = g_projection * g_view * g_model * position;

	g_projected_position[0] /= g_projected_position[3];
	g_projected_position[1] /= g_projected_position[3];
	g_projected_position[2] /= g_projected_position[3];

	g_world_normal = normalize(g_view * g_model * g_normal);

	g_world_position = (g_view * g_model * position);

	return;
}
