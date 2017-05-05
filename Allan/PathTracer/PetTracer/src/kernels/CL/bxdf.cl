#ifndef BXDF_CL
#define BXDF_CL

#include <scene.cl>
#include <material.cl>

#define DENOM_EPS 0.0007f

/// Full Fresnel equations
float FresnelDielectric( float etai, float etat, float ndotwi, float ndotwt )
{
	// Parallel and perpendicular polarization
	float rparl = ( ( etat * ndotwi ) - ( etai * ndotwt ) ) / ( ( etat * ndotwi ) + ( etai * ndotwt ) );
	float rperp = ( ( etai * ndotwi ) - ( etat * ndotwt ) ) / ( ( etai * ndotwi ) + ( etat * ndotwt ) );
	return ( rparl*rparl + rperp*rperp ) * 0.5f;
}

// Schlicks approximation of fresnel equation
float SchlickFresnel(float eta, float ndotw)
{
	const float f = ( ( 1.0f - eta ) / ( 1.0f + eta ) );
	const float f0 = f * f;
	const float m = 1.0f - fabs( ndotw );
	const float m2 = m * m;
	return f0 + ( 1.0f - f0 ) * m2 * m2 * m;
}

float3 SchlickFresnel3( float3 f0, float ndotw )
{
	const float m = 1.0f - fabs( ndotw );
	const float m2 = m * m;
	return f0 + ( 1.0f - f0 ) * m2 * m2 * m;
}

float MicrofacetDistribution_GGX_D( float roughness, float3 m )
{
	float ndotm = fabs( m.y );
	float ndotm2 = ndotm * ndotm;
	float sinmn = native_sqrt( 1.f - clamp( ndotm * ndotm, 0.f, 1.f ) );
	float tanmn = ndotm > DENOM_EPS ? sinmn / ndotm : 0.f;
	float a2 = roughness * roughness;
	float denom = ( PI * ndotm2 * ndotm2 * ( a2 + tanmn * tanmn ) * ( a2 + tanmn * tanmn ) );
	return denom > DENOM_EPS ? ( a2 / denom ) : 0.f;
}

float MicrofacetDistribution_GGX_GetPdf(
	// Halfway vector
	float3 m,
	// Rougness
	float roughness,
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Outgoing direction
	float3 wo
)
{
	float mpdf = MicrofacetDistribution_GGX_D( roughness, m ) * fabs( m.y );
	// See Humphreys and Pharr for derivation
	float denom = ( 4.f * fabs( dot( wo, m ) ) );

	return denom > DENOM_EPS ? mpdf / denom : 0.f;
}

void MicrofacetDistribution_GGX_Sample(
	// Roughness
	float roughness,
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Sample
	float2 sample,
	// Outgoing  direction
	float3* wo,
	// PDF at wo
	float* pdf
)
{
	float r1 = sample.x;
	float r2 = sample.y;

	// Sample halfway vector first, then reflect wi around that
	float temp = atan( roughness * native_sqrt( r1 ) / native_sqrt( 1.f - r1 ) );
	//float temp = sqrt( (1 -  r1)  * native_recip( r1 * ( roughness * roughness - 1.0f ) + 1.0f ) ) ;
	float theta = ( float ) ( ( temp >= 0 ) ? temp : ( temp + 2 * PI ) );

	float costheta = native_cos( theta );
	float sintheta = native_sqrt( 1.f - clamp( costheta * costheta, 0.f, 1.f ) );

	// phi = 2*PI*ksi2
	float cosphi = native_cos( 2.f*PI*r2 );
	float sinphi = native_sqrt( 1.f - clamp( cosphi * cosphi, 0.f, 1.f ) );

	// Calculate wh
	float3 wh = makeFloat3( sintheta * cosphi, costheta, sintheta * sinphi );

	// Reflect wi around wh
	*wo = -wi + 2.f*fabs( dot( wi, wh ) ) * wh;

	// Calc pdf
	*pdf = MicrofacetDistribution_GGX_GetPdf( wh, roughness, dg, wi, *wo );
}

float MicrofacetDistribution_GGX_G1( float roughness, float3 v, float3 m )
{
	float ndotv = fabs( v.y );
	float mdotv = fabs( dot( m, v ) );

	float sinnv = native_sqrt( 1.f - clamp( ndotv * ndotv, 0.f, 1.f ) );
	float tannv = ndotv > 0.f ? sinnv / ndotv : 0.f;
	float a2 = roughness * roughness;
	return 2.f / ( 1.f + native_sqrt( 1.f + a2 * tannv * tannv ) );
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_GGX_G( float roughness, float3 wi, float3 wo, float3 wh )
{
	return MicrofacetDistribution_GGX_G1( roughness, wi, wh ) * MicrofacetDistribution_GGX_G1( roughness, wo, wh );
}

float3 MicrofacetGGX_Evaluate(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Outgoing direction
	float3 wo
)
{
	const float3 ks = dg->mat.specular.xyz;
	const float roughness = 0.1f;// dg->mat.roughness;

	// Incident and reflected zenith angles
	float costhetao = fabs( wo.y );
	float costhetai = fabs( wi.y );

	// Calc halfway vector
	float3 wh = normalize( wi + wo );

	float3 F = SchlickFresnel3( ks, dot( wo, wh ) );

	float denom = ( 4.f * costhetao * costhetai );

	// F(eta) * D * G * ks / (4 * cosa * cosi)
	return denom > 0.f ? F * MicrofacetDistribution_GGX_G( roughness, wi, wo, wh ) * MicrofacetDistribution_GGX_D( roughness, wh ) / denom : 0.f;
}

float3 MicrofacetGGX_Sample(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Sample
	float2 sample,
	// Outgoing direction
	float3* wo,
	// PDF at wo
	float* pdf
)
{
	const float roughness = dg->mat.roughness;

	MicrofacetDistribution_GGX_Sample( roughness, dg, wi, sample, wo, pdf );

	return MicrofacetGGX_Evaluate( dg, wi, *wo );

}

float3 Bxdf_Sample(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// RGN
	float2 sample,
	// Outgoing direction
	float3* wo,
	// Pdf at w
	float* pdf
)
{
	float3 wi_local = normalize(DifferentialGeometry_ToTangentSpace( dg, wi ));
	float3 wo_local;

	float3 bxdfSample = 0.0f;


	bxdfSample = MicrofacetGGX_Sample( dg, wi_local, sample, &wo_local, pdf );


	*wo = normalize(DifferentialGeometry_ToWorldSpace( dg, wo_local ));
	return bxdfSample;


}

float BalanceHeuristic( int nf, float fpdf, int ng, float gpdf )
{
	float f = nf * fpdf;
	float g = ng * gpdf;
	return ( f ) / ( f + g );
}

#endif