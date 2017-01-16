#define _WIN32_WINNT 0x0500
#include <iostream>
#include <fstream>
#include <windows.h>
#include "sphere.h"
#include "hitablelist.h"
#include "float.h"
#include "camera.h"
#include <random>

int nx = 800;
int ny = 400;
int ns = 16;
int amp = 1;
int bounces = 10;

vec3 color(const ray& r, hitable *world, int depth)
{
    hit_record rec;
    if(world->hit(r, 0.001, FLT_MAX, rec))
    {
        ray scattered;
        vec3 attenuation;
        if(depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        {
            return attenuation*color(scattered, world, depth+1);
        }
        else
        {
            return vec3(0,0,0);
        }
    }
    else
    {
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5*(unit_direction.y() + 1.0);
        return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
    }
}

vec3 colorIt(const ray& r, hitable *world, int depth)
{
    //attenuation corresponde ao albedo da cor.
    ray r1 = r;
    int dep = depth;
    vec3 attenuation;

    hit_record rec;

    //Iniciar a cor com a cor do fundo.
    vec3 unit_direction = unit_vector(r1.direction());
    float t = 0.5*(unit_direction.y() + 1.0);
    vec3 col = (1.0 - t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);

    //Reflete o raio um numero finito de vezes.
    for(int i = 1; i <= bounces; i++)
    {
        if(world->hit(r1, 0.001, FLT_MAX, rec))
        {
            ray scattered;
            if(dep < 50 && rec.mat_ptr->scatter(r1, rec, attenuation, scattered))
            {
                r1 = scattered;
                dep++;
                col *= attenuation;
            }
            else
            {
                col = vec3(0.0, 0.0, 0.0);
                return col;
            }
        }
        //Se o raio não atingir nada duante a iteracao retorna a cor inicial.
        else
        {
            return col;
        }
    }
    //Se apos a iteracao o raio nao atingir nada.
    col = vec3(0.0, 0.0, 0.0);
    return col;
}


int main()
{
    HWND myconsole = GetConsoleWindow();
    HDC mydc = GetDC(myconsole);

    std::ofstream out;
    out.open("teste.ppm");
    out << "P3\n" << nx << " " << ny << "\n255\n";

    MoveWindow(myconsole, 100, 100, nx, ny, TRUE);

    vec3 lower_left_corner(-2.0, -1.0, -1.0);
    vec3 horizontal(4.0, 0.0, 0.0);
    vec3 vertical(0.0, 2.0, 0.0);
    vec3 origin(0.0, 0.0, 0.0);

    float R = cos(M_PI/4);

    hitable *list[5];
    list[0] = new sphere(vec3(0, 0, -1), 0.5, new lambertian(vec3(0.1, 0.2, 0.5)));
    list[1] = new sphere(vec3(0, -100.5, -1), 100, new lambertian(vec3(0.8, 0.8, 0.0)));
    list[2] = new sphere(vec3(1, 0, -1), 0.5, new metal(vec3(0.8, 0.6, 0.3), 0.2));
    list[3] = new sphere(vec3(-1, 0, -1), 0.5, new dieletric(1.5));
    list[4] = new sphere(vec3(-1, 0, -1), -0.45, new dieletric(1.5));
    hitable *world = new hitable_list(list, 5);

    vec3 lookfrom(0,1,5);
    vec3 lookat(0,0,-1);
    float dist_to_focus = (lookfrom - lookat).length();
    float aperture = 0.5;

    camera cam = camera(lookfrom, lookat, vec3(0,1,0), 20, float(nx)/float(ny), aperture, dist_to_focus);

    while(true)
    for(int j = ny - 1; j >= 0; j--)
    {
        for(int i = 0; i < nx; i++)
        {
            vec3 col(0, 0, 0);
            for(int s = 0; s < ns; s++)
            {
                float u = float(i + getRand(amp)) / float(nx);
                float v = float(j + getRand(amp)) / float(ny);
                ray r = cam.get_ray(u, v);
                vec3 p = r.point_at_parameter(2.0);
                col += color(r, world, 0);
            }
            col /= float(ns);
            col = vec3( sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
            int ir = int(255.99*col[0]);
            int ig = int(255.99*col[1]);
            int ib = int(255.99*col[2]);

            SetPixel(mydc,i,abs(j-(ny-1)),RGB(ir, ig, ib));
            out << ir << " " << ig << " " << ib << "\n";
        }
    }
    out.close();
    ReleaseDC(myconsole, mydc);
    std::cin.ignore();
}
