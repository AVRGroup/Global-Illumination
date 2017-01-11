#version 330 core

// FRAGMENT SHADER

#define M_PI 3.1415926535897932384626433832795
#define maxFloat 3.402823e+38

layout(location=0) out vec4 vFragColor;

// STRUCTS

// Struct com os parametros da camera.
struct Camera
{
    vec3 lookfrom;
    vec3 lookat;
    vec3 vup;
    float vfov;
    float aspect;
    float aperture;
    float focus_dist;
};

// Raio simples com origem e direcao.
struct Ray
{
    vec3 origin;
    vec3 direction;
};

// Armazena a informacao do ponto de colisao do raio.
struct HitRecord
{
    float t;
    vec3 p;
    vec3 normal;
    vec3 albedo;
    float fuzz;
};


// VARIAVEIS

// Uniforms
//uniform float width;
//uniform float height;
//uniform float addBallX;
//uniform float addBallY;
//uniform float addBallZ;
//uniform int samplesAA;

float width = 800;
float height = 600;
float addBallX = 0;
float addBallY = 0;
float addBallZ = 5;
int samplesAA = 16;

// Parametros
int blurAA = 1;
int bounces = 20;
float seed = 1;
//int samplesAA = 16;

// Auxiliares
HitRecord rec = HitRecord(0.0, vec3(0,0,0), vec3(0,0,0), vec3(0,0,0), 1.0);
vec3 col = vec3(0.0, 0.0, 0.0);
Ray ray;
vec3 attenuation = vec3(0,0,0);
vec3 reflected;
vec3 refracted;
int index;

// Vetores com os parametros das esferas.

// Centros das esferas.
vec3 center[4];
// Raios das esferas.
float radius[4];
// Albedo das esferas. Fracao de luz refletida por elas.
vec3 albedo[4];
// Parametro para definicao dos reflexos.
float fuzz[4];
// Materiais. 1 - Difuso, 2 - Reflexivo, 3 - Rafratario.
int material[4];
// Indices de rafracao.
float refIdx[4];


// FUNCOES AUXILIARES

// Retorna um numero aleatorio.
float rand()
{
    seed++;
    return fract(sin(dot(gl_FragCoord.xyz + seed, vec3(12.9898, 78.233, 151.7182))) * 43758.5453 + seed);
}


// Retorna o ponto no raio correspondente ao parametro t.
vec3 point_at_parameter(float t, Ray r)
{
    return r.origin + t*r.direction;
}


// Retorna um ponto dentro da esfera unitaria.
vec3 random_in_unit_sphere()
{
    float u = rand();
    float v = rand();
    float r = sqrt(u);
    float angle = 6.283185307179586 * v;
    vec3 sdir, tdir;
    if (abs(rec.normal.x)<0.5)
    {
        sdir = cross(rec.normal, vec3(1,0,0));
    }
    else
    {
        sdir = cross(rec.normal, vec3(0,1,0));
    }
    tdir = cross(rec.normal, sdir);
    return r*cos(angle)*sdir + r*sin(angle)*tdir + sqrt(1.-u)*rec.normal;
}


// Retorna um ponta aleatorio em um disco.
vec3 random_in_unit_disk()
{
    vec3 p;
    do {
        p = 2.0*vec3(rand(), rand(), 0) - vec3(1, 1, 0);
    } while (dot(p,p) >= 1.0);
    return p;
}


// Reflete um raio.
vec3 reflect(vec3 v, vec3 n)
{
    return v - 2*dot(v,n)*n;
}

// Faz a refracao de um raio.
bool refract(vec3 n, float ni_over_nt)
{
    vec3 v = ray.direction;
    vec3 uv = normalize(v);
    vec3 un = normalize(n);
    float dt = dot(uv, un);
    float discriminant = 1.0 - ni_over_nt*ni_over_nt*(1-dt*dt);
    if(discriminant > 0)
    {
        refracted = ni_over_nt*(uv - un*dt) - un*sqrt(discriminant);
        return true;
    }
    else
        return false;
}


float schlick(float cosine)
{
    float ref_idx = refIdx[index];
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine), 5);
}


// Cria um raio a partir dos parametros da camera.
void createRayFromCamera(float i, float j, Camera cam)
{
    float lens_radius = cam.aperture/2;
    vec3 u, v, w;
    float theta = cam.vfov*M_PI/180;
    float half_height = tan(theta/2);
    float half_width = cam.aspect * half_height;
    w = normalize(cam.lookfrom - cam.lookat);
    u = normalize(cross(cam.vup, w));
    v = cross(w, u);
    vec3 lower_left_corner = cam.lookfrom - half_width*cam.focus_dist*u - half_height*cam.focus_dist*v - cam.focus_dist*w;
    vec3 horizontal = 2*half_width*cam.focus_dist*u;
    vec3 vertical = 2*half_height*cam.focus_dist*v;
    vec3 rd = lens_radius*random_in_unit_disk();
    vec3 offset = u*rd.x + v*rd.y;
    ray = Ray(cam.lookfrom + offset, lower_left_corner + i*horizontal + j*vertical - cam.lookfrom - offset);
}

// FUNCOES PRINCIPAIS

// Determina o cmportamento do raio de acordo com a superficie.
bool scatter()
{
    int mat = material[index];
    switch(mat)
    {
        // Lambertiano
        case 1:
            vec3 target = rec.p + rec.normal + random_in_unit_sphere();
            ray = Ray(rec.p, target-rec.p);
            attenuation = rec.albedo;
            return true;
        // Metalico
        case 2:
            reflected = reflect(normalize(ray.direction), rec.normal);
            ray = Ray(rec.p, reflected + rec.fuzz*random_in_unit_sphere());
            attenuation = rec.albedo;
            return (dot(ray.direction, rec.normal) > 0);
        // Dieletrico
        case 3:
            vec3 outward_normal;
            reflected = reflect(ray.direction, rec.normal);
            float ni_over_nt;
            attenuation = vec3(1.0, 1.0, 1.0);
            float reflect_prob;
            float cosine;
            if(dot(ray.direction, rec.normal) > 0)
            {
                outward_normal = -rec.normal;
                ni_over_nt = refIdx[index];
                cosine = refIdx[index] * dot(ray.direction, rec.normal) / ray.direction.length();
            }
            else
            {
                outward_normal = rec.normal;
                ni_over_nt = 1.0/refIdx[index];
                cosine = -dot(ray.direction, rec.normal) / ray.direction.length();
            }
            if(refract(outward_normal, ni_over_nt))
            {
                reflect_prob = schlick(cosine);
            }
            else
            {
                ray = Ray(rec.p, reflected);
                reflect_prob = 1.0;
            }
            if(rand() < reflect_prob)
            {
                ray = Ray(rec.p, reflected);
                //attenuation = rec.albedo;
            }
            else
            {
                ray = Ray(rec.p, refracted);
                attenuation = rec.albedo;
            }
            return true;
    }
}


// Identifica quando um raio intercepta uma esfera.
// Nao calcular para valores negativos melhora a aparencia e desempenho.
// A versao de CPU so funciona para a intersecao positiva.
bool hit(float t_min, float t_max, int i)
{
    vec3 oc = ray.origin - center[i];
    float a = dot(ray.direction, ray.direction);
    float b = dot(oc, ray.direction);
    float c = dot(oc, oc) - radius[i]*radius[i];
    float discriminant = b*b - a*c;
    if(discriminant > 0.0)
    {
        float temp = (-b - sqrt(b*b-a*c))/a;
        if(temp < t_max && temp > t_min)
        {
            rec.t = temp;
            rec.p = point_at_parameter(rec.t, ray);
            rec.normal = (rec.p - center[i]) / radius[i];
            rec.albedo = albedo[i];
            rec.fuzz = fuzz[i];
            return true;
        }
        temp = (-b + sqrt(b*b-a*c))/a;
        if(temp < t_max && temp > t_min)
        {
            rec.t = temp;
            rec.p = point_at_parameter(rec.t, ray);
            rec.normal = (rec.p - center[i]) / radius[i];
            rec.albedo = albedo[i];
            rec.fuzz = fuzz[i];
            return true;
        }
    }
    return false;
}


// Percorre o vetor de objetos e retorna verdadeiro se o raio atingir algo.
bool hitWorld(float t_min, float t_max)
{
    bool hit_anything = false;
    float closest_so_far = t_max;
    for(int i = 0; i < center.length(); i++)
    {
        if(hit(t_min, closest_so_far, i))
        {
            index = i;
            hit_anything = true;
            closest_so_far = rec.t;
        }
    }
    return hit_anything;
}


// Retorna a cor obtida pelo raio.
vec3 color(int depth)
{
    // Iniciar a cor com a cor do fundo.
    vec3 unit_direction = normalize(ray.direction);
    float t = 0.5*(unit_direction.y + 1.0);
    vec3 col = (1.0 - t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);

    // Reflete o raio um numero finito de vezes.
    for(int i = 1; i <= bounces; i++)
    {
        if(hitWorld(0.001, maxFloat))
        {
            if(scatter())
            {
                col *= attenuation;
            }
            else
            {
                col = vec3(0.0, 0.0, 0.0);
                return col;
            }
        }
        // Se o raio n�o atingir nada duante a iteracao retorna a cor inicial.
        else
        {
            return col;
        }
    }
    // Se apos a iteracao o raio nao atingir nada.
    col = vec3(0.0, 0.0, 0.0);
    return col;
}


// MAIN

void main(void)
{
    // Esfera central.
    center[0] = vec3(0.0, 0.0, 0.0);
    radius[0] = 0.5;
    albedo[0] = vec3(0.9,0.9,0.9);
    fuzz[0] = 1.0;
    material[0] = 1;
    refIdx[0] = 0;

    // Esfera Direita.
    center[1] = vec3(1.0, 0.0, 1.0);
    radius[1] = 0.5;
    albedo[1] = vec3(0.8,0.6,0.2);
    fuzz[1] = 0.3;
    material[1] = 2;
    refIdx[1] = 0;

    // Esfera Esquerda.
    center[2] = vec3(-1.0, 0.0, -1.0);
    radius[2] = 0.5;
    albedo[2] = vec3(0.7,0.9,0.8);
    fuzz[2] = 0.1;
    material[2] = 3;
    refIdx[2] = 1.01;

    // Esfera solo.
    center[3] = vec3(0.0, -100.5, 0.0);
    radius[3] = 100.0;
    albedo[3] = vec3(0.5,0.5,0.5);
    fuzz[3] = 1.0;
    material[3] = 1;
    refIdx[3] = 0;

    // Inicializando a camera.
    vec3 lookfrom = vec3(0.0 + addBallX, 0.0 + addBallY, 0.0 + addBallZ);
    vec3 lookat = vec3(0,0,0);
    float dist_to_focus = (lookfrom - lookat).length();
    float aperture = 0.1;
    Camera camera = Camera(lookfrom, lookat, vec3(0,1,0), 45, float(width)/float(height), aperture, dist_to_focus);

    // Iteracao AA
    for(int i = 0; i < samplesAA; i++)
    {
        float x = gl_FragCoord.x;
        float y = gl_FragCoord.y;
        vec2 uv = vec2((x + blurAA * rand())/width, (y + blurAA * rand())/height);
        createRayFromCamera(uv.x, uv.y, camera);
        col += vec3(color(0));
    }

    col /= float(samplesAA);
    col = vec3(sqrt(col.x), sqrt(col.y), sqrt(col.z));
    vFragColor = vec4(col, 1.0);
}
