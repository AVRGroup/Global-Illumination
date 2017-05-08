#ifndef BXDF_CL
#define BXDF_CL

#include <scene.cl>
#include <material.cl>

#define DENOM_EPS 1e-8f

float3 SchlickFresnel(float3 r0, float WioN)
{
	float m = (1 - WioN);
	float m2 = m * m;
	return r0 + (1.0f - r0)*(m2*m2*m);
}

/*
 Lambert BRDF
 */
 /// Lambert BRDF evaluation
float3 Lambert_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo
    )
{
    const float3 kd = dg->mat.albedo.xyz;

    return kd / PI;
}

/// Lambert BRDF PDF
float Lambert_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo
    )
{
    return fabs(wo.y) / PI;
}

/// Lambert BRDF sampling
float3 Lambert_Sample(
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
    const float3 kd = dg->mat.albedo.xyz;

    float sinphi = native_sin(2*PI*sample.x);
    float cosphi = native_cos(2*PI*sample.x);
    float costheta = native_sqrt(1.0f - sample.y);
    float sintheta = native_sqrt(1.f - costheta * costheta);
    
    // Return the result
    *wo = normalize(makeFloat3(sintheta * cosphi, costheta, sintheta * sinphi));

    *pdf = fabs(wo->y) / PI;

    return kd / PI;
}

/** GGX **/
 // Distribution fucntion
float MicrofacetDistribution_GGX_D(float roughness, float3 m)
{
    float ndotm = fabs(m.y);
    float ndotm2 = ndotm * ndotm;
    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
    float tanmn = sinmn / ndotm;
    float a2 = roughness * roughness;
    float denom = (PI * ndotm2 * ndotm2 * (a2 + tanmn * tanmn) * (a2 + tanmn * tanmn));
    return denom > DENOM_EPS ? (a2 / denom) : 0.f;
}

float MicrofacetDistribution_GGX_G1(float roughness, float3 v, float3 m)
{
    float ndotv = fabs(v.y);
    float mdotv = fabs(dot(m, v));

    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = sinnv / ndotv;
    float a2 = roughness * roughness;
    return 2.f / (1.f + native_sqrt(1.f + a2 * tannv * tannv));
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_GGX_G(float roughness, float3 wi, float3 wo, float3 wh)
{
    return MicrofacetDistribution_GGX_G1(roughness, wi, wh) * MicrofacetDistribution_GGX_G1(roughness, wo, wh);
}

// PDF of the given direction
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
    float mpdf = MicrofacetDistribution_GGX_D(roughness, m) * fabs(m.y);
    // See Humphreys and Pharr for derivation
    float denom = (4.f * fabs(dot(wo, m)));

	float pdf = denom > DENOM_EPS ? mpdf / denom : 0.f;
    return pdf > DENOM_EPS ? pdf : 0.0f;
}


float3 MicrofacetGGX_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
	// Half-Vector
	float3 wh,
	// Roughness
	float roughness
    )
{
    const float3 ks =/* makeFloat3(0.04, 0.04, 0.04);//*/dg->mat.specular.xyz;

    // Incident and reflected zenith angles
    float costhetao = fabs(wo.y);
    float costhetai = fabs(wi.y);

    float3 F = 1.0;//SchlickFresnel(ks, clamp(dot(wi, wh), 0.0f, 1.0f));

    float denom = (4.f * costhetao * costhetai);

    float3 res = denom > 0.f ? F * ks * MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom : 0.f;
    if (length(res) < 0.1f)
        res = 0.f;
    return res;
}

float3 MicrofacetGGX_Sample(
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
	const float roughness = 0.8f;

	// Sample half-vector
	float theta = atan2(roughness * native_sqrt(sample.x), native_sqrt(1.0f - sample.x));
	float costheta = native_cos(theta);
	float sintheta = native_sin(theta);

	float phi = 2.0f * PI * sample.y;
	float sinphi = native_sin(phi);
	float cosphi = native_cos(phi);
	float3 wh = makeFloat3( sintheta * cosphi, costheta, sintheta * sinphi );

	*wo = 2.0f*wh*fabs(dot(wi, wh)) - wi;

	*pdf = MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, *wo);

	return MicrofacetGGX_Evaluate(dg, wi, *wo, wh, roughness);
}

float3 Bxdf_Sample(
	// Geometry
	DifferentialGeometry *dg,
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
	float3 wi_local = matrixMulVector3(dg->worldToTangent, wi);
	float3 wo_local;

	//dg->mat.fresnel = 1.0f;//length(SchlickFresnel(makeFloat3(0.04f, 0.04f, 0.04f), wi_local.y));

	float3 bxdfSample = 0.0f;


	bxdfSample = MicrofacetGGX_Sample( dg, wi_local, sample, &wo_local, pdf );
	//bxdfSample = Lambert_Sample(dg, wi_local, sample, &wo_local, pdf);


	*wo = matrixMulVector3(dg->tangentToWorld, wo_local);
	return bxdfSample;
}

float BalanceHeuristic( int nf, float fpdf, int ng, float gpdf )
{
	float f = nf * fpdf;
	float g = ng * gpdf;
	return ( f ) / ( f + g );
}

#endif