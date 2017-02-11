// v1, v2 e v3 são os vertices do triangulo
int IntersectTriangle(Ray r, vec3 v1, vec3 v2, vec3 v3 )
{
	vec3 e1 = v2 - v1;
	vec3 e2 = v3 - v1;
	vec3 d = r.origin - v1;
	vec3 s1 = cross( r.direction, e2 );
	vec3 s2 = cross( d, e1 );
	float invd = 1.0f / ( dot( e1, s1 ) );
	float b1 = dot( d, s1 ) * invd;
	float b2 = dot( r.direction, s2 ) * invd;
	float temp = dot( e2, s2 ) * invd;

	if(b1 < 0.0f || b1 > 1.0f || b2 < 0.0f || b2 > 1.0f || b1 + b2 > 1.0f
	{
		return 0;
	}
	else
	{
		// b1 e b2 são respectivamente u, v do triangulo
		// (para ter as coordenadas finais de textura deve interpolar as coordenadas de cada vertice por uv)
		// UVf = lerp(uv[v1], uv[v2], u) + lerp(uv[v1], uv[v3], v)
		// temp é o valor de t do raio
		intersec.uvwt = vec4( b1, b2, 0, temp );
		return 1;
	}
}
