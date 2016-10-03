/* 
 * File:   raycast.c
 * Author: Matthew
 *
 * Created on September 29, 2016, 11:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#define SPHERE 0
#define PLANE 1
#define HEIGHT 100
#define WIDTH 100
#define MAXCOLOR 255

typedef struct {
    int kind;
    double color[3];
    double position[3];
    union {
        struct {
            double radius;
        } sphere;
        struct {
            double normal[3];
        } plane;
    };
} Object;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pixel;

static inline double sqr(double v) {
    return v*v;
}

static inline void normalize(double* v) {
    double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
    
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}

void read_scene(char*);
void set_camera(FILE*);
double sphere_intersect(double*, double*, double*, double);
double plane_intersect(double*, double*, double*, double*);
void skip_ws(FILE*);
void expect_c(FILE*, int);
int next_c(FILE*);
char* next_string(FILE*);
double next_number(FILE*);
double* next_vector(FILE*);
void output_p6();

int line = 1;
Object** objects;
Pixel* pixmap;

// camera
double h = 0.7;
double w = 0.7;

/*
 * 
 */
int main(int argc, char** argv) {
    objects = malloc(sizeof(Object*)*129);
    read_scene("test.json");
    
    // more camera
    double cx = 0;
    double cy = 0;
    
    int M = HEIGHT;
    int N = WIDTH;
    
    double pixheight = h/M;
    double pixwidth = w/N;
    
    pixmap = malloc(sizeof(Pixel)*M*N);
    int index = 0;
    for (int y=0; y<M; y++) {
        for (int x=0; x<N; x++) {
            double Ro[3] = {cx, cy, 0};
            double Rd[3] = {cx - (w/2) + pixwidth*(x + 0.5),
                            cy - (h/2) + pixheight*(y + 0.5),
                            1};
            normalize(Rd);
            
            double best_t = INFINITY;
            Object* object;
            for (int i=0; objects[i] != NULL; i++) {
                double t = 0;
                
                switch (objects[i]->kind) {
                    case 0:
                        t = sphere_intersect(Ro, Rd, objects[i]->position, objects[i]->sphere.radius);
                        break;
                    case 1:
                        t = plane_intersect(Ro, Rd, objects[i]->position, objects[i]->plane.normal);
                        break;
                    default:
                        fprintf(stderr, "Error: Unknown object.\n");
                        exit(1);
                }
                
                if (t > 0 && t < best_t) {
                    best_t = t;
                    object = malloc(sizeof(Object));
                    memcpy(object, objects[i], sizeof(Object));
                }
            }
            
            if (best_t > 0 && best_t != INFINITY) {
                pixmap[index].r = (unsigned char)(object->color[0]*MAXCOLOR);
                pixmap[index].g = (unsigned char)(object->color[1]*MAXCOLOR);
                pixmap[index].b = (unsigned char)(object->color[2]*MAXCOLOR);
            } else {
                pixmap[index].r = 255;
                pixmap[index].g = 255;
                pixmap[index].b = 255;
            }
            
            index++;
        }
    }
    
    FILE* output = fopen("ouput.ppm", "w");
    output_p6(output, M, N);
    
    return (EXIT_SUCCESS);
}

void read_scene(char* filename) {
    int c;
    FILE* json = fopen(filename, "r");
    
    if (json == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'.\n", filename);
        exit(1);
    }
    
    skip_ws(json);
    expect_c(json, '[');
    skip_ws(json);
    
    int i = 0;
    while (1) {
        c = next_c(json);
        if (c == ']') {
            fprintf(stderr, "Error: This scene is empty.\n");
            fclose(json);
            objects[i] = NULL;
            return;
        }
        
        if (c == '{') {
            objects[i] = malloc(sizeof(Object));
            
            skip_ws(json);
            
            char* key = next_string(json);
            if (strcmp(key, "type") != 0) {
                fprintf(stderr, "Error: Expected 'type' key on line number %d.\n", line);
                exit(1);
            }
            
            skip_ws(json);
            expect_c(json, ':');
            skip_ws(json);
            
            
            char* value = next_string(json);
            if (strcmp(value, "camera") == 0) {
                set_camera(json);
            } else if (strcmp(value, "sphere") == 0) {
                objects[i]->kind = SPHERE;
            } else if (strcmp(value, "plane") == 0) {
                objects[i]->kind = PLANE;
            } else {
                fprintf(stderr, "Error: Unknown type, '%s', on line number %d.\n", value, line);
                exit(1);
            }
            
            skip_ws(json);
            
            if (strcmp(value, "camera") != 0) {
                while (1) {
                    c = next_c(json);
                    if (c == '}') {
                        break;
                    } else if (c == ',') {
                        skip_ws(json);
                        char* key = next_string(json);

                        skip_ws(json);
                        expect_c(json, ':');
                        skip_ws(json);

                        if (strcmp(key, "radius") == 0) {
                            double value = next_number(json);
                            if (objects[i]->kind == SPHERE) {
                                if (strcmp(key, "radius") == 0) {
                                    objects[i]->sphere.radius = value;
                                } else {
                                    fprintf(stderr, "Error: Unknown property '%s' for 'sphere' on line number %d.\n", key, line);
                                    exit(1);
                                }
                            } else if (objects[i]->kind == PLANE) {
                                fprintf(stderr, "Error: Unknown property '%s' for 'plane' on line number %d.\n", key, line);
                                    exit(1);
                            }
                        } else if ((strcmp(key, "color") == 0) || 
                                   (strcmp(key, "position") == 0) || 
                                   (strcmp(key, "normal") == 0)) {
                            double* value = next_vector(json);
                            if (objects[i]->kind == SPHERE) {
                                if (strcmp(key, "color") == 0) {
                                    objects[i]->color[0] = value[0];
                                    objects[i]->color[1] = value[1];
                                    objects[i]->color[2] = value[2];
                                } else if (strcmp(key, "position") == 0) {
                                    objects[i]->position[0] = value[0];
                                    objects[i]->position[1] = value[1];
                                    objects[i]->position[2] = value[2];
                                } else {
                                    fprintf(stderr, "Error: Unknown property '%s' for 'sphere' on line number %d.\n", key, line);
                                    exit(1);
                                }
                            } else if (objects[i]->kind == PLANE) {
                                if (strcmp(key, "color") == 0) {
                                    objects[i]->color[0] = value[0];
                                    objects[i]->color[1] = value[1];
                                    objects[i]->color[2] = value[2];
                                } else if (strcmp(key, "position") == 0) {
                                    objects[i]->position[0] = value[0];
                                    objects[i]->position[1] = value[1];
                                    objects[i]->position[2] = value[2];
                                } else if (strcmp(key, "normal") == 0) {
                                    objects[i]->plane.normal[0] = value[0];
                                    objects[i]->plane.normal[1] = value[1];
                                    objects[i]->plane.normal[2] = value[2];
                                } else {
                                    fprintf(stderr, "Error: Unknown property '%s' for 'plane' on line number %d.\n", key, line);
                                    exit(1);
                                }
                            }
                        } else {
                            fprintf(stderr, "Error: Unknown property '%s' on line number %d.\n", key, line);
                            exit(1);
                        }

                        skip_ws(json);
                    } else {
                        fprintf(stderr, "Error: Unexpected value on line number %d.\n", line);
                        exit(1);
                    }
                }

                i++;
            }
            
            skip_ws(json);
            c = next_c(json);
            if (c == ',') {
                skip_ws(json);
            } else if (c == ']') {
                objects[i] = NULL;
                fclose(json);
                return;
            } else {
                fprintf(stderr, "Error: Expecting ',' or ']' on line number %d.\n", line);
                exit(1);
            }
            if (i == 129) {
                objects[i] = NULL;
                fclose(json);
                fprintf(stderr, "Error: Too many objects in file.\n");
                return;
            }
            
        } else {
            fprintf(stderr, "Error: Expecting '{' on line number %d.\n", line);
            exit(1);
        }
    }
}

void set_camera(FILE* json) {
    int c;
    skip_ws(json);
    
    while (1) {
        c = next_c(json);
        if (c == '}') {
            break;
        } else if (c == ',') {
            skip_ws(json);
            char* key = next_string(json);
            
            skip_ws(json);
            expect_c(json, ':');
            skip_ws(json);
            double value = next_number(json);
            if (strcmp(key, "width") == 0) {
                w = value;
            } else if (strcmp(key, "height") == 0) {
                h = value;
            } else {
                fprintf(stderr, "Error: Unknown property '%s' for 'camera' on line number %d.\n", key, line);
                exit(1);
            }
        }
    }
}

double sphere_intersect(double* Ro, double* Rd, double* C, double r) {
    double a = sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]);
    double b = 2*(Rd[0]*(Ro[0]-C[0]) + Rd[1]*(Ro[1]-C[1]) + Rd[2]*(Ro[2]-C[2]));
    double c = sqr(Ro[0]-C[0]) + sqr(Ro[1]-C[1]) + sqr(Ro[2]-C[2]) - sqr(r);
    
    double det = sqr(b) - 4*a*c;
    if (det < 0)
        return -1;
    
    det = sqrt(det);
    
    double t0 = (-b - det) / (2*a);
    if (t0 > 0)
        return t0;
    
    double t1 = (-b + det) / (2*a);
    if (t1 > 0)
        return t1;
    
    return -1;
}

double plane_intersect(double* Ro, double* Rd, double* P, double* N) {
    double d = N[0]*P[0] + N[1]*P[1] + N[2]*P[2];
    double t = -(N[0]*Ro[0] + N[1]*Ro[1] + N[2]*Ro[2] + d) / (N[0]*Rd[0] + N[1]*Rd[1] + N[2]*Rd[2]);
    
    if (t > 0)
        return t;
    
    return -1;
}

void skip_ws(FILE* json) {
    int c = next_c(json);
    
    while(isspace(c))
        c = next_c(json);
    
    ungetc(c, json);
}

void expect_c(FILE* json, int d) {
    int c = next_c(json);
    
    if (c == d)
        return;
    
    fprintf(stderr, "Error: Expected '%c' on line number %d.\n", d, line);
    exit(1);
}

int next_c(FILE* json) {
    int c = fgetc(json);
    
    if (c == '\n')
        line++;
    
    if (c == EOF) {
        fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
        exit(1);
    }
    
    return c;
}

char* next_string(FILE* json) {
    char buffer[129];
    int c = next_c(json);
    
    if (c != '"') {
        fprintf(stderr, "Error: Expected string on line number %d.\n", line);
        exit(1);
    }
    
    c = next_c(json);
    
    int i = 0;
    while (c != '"') {
        if (i >= 128) {
            fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
            exit(1);
        }
        
        if (c == '\\') {
            fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
            exit(1);
        }
        
        if (c < 32 || c > 126) {
            fprintf(stderr, "Error: Strings may only contain ASCII characters.\n");
            exit(1);
        }
        
        buffer[i] = c;
        i++;
        c = next_c(json);
    }
    
    buffer[i] = 0;
    return strdup(buffer);
}

double next_number(FILE* json) {
    double value;
     if (fscanf(json, "%f", &value) != 1) {
         fprintf(stderr, "Error: Number value not found on line number %d.\n", line);
         exit(1);
     }
    return value;
}

double* next_vector(FILE* json) {
    double* v = malloc(3*sizeof(double));
    
    expect_c(json, '[');
    skip_ws(json);
    v[0] = next_number(json);
    skip_ws(json);
    
    expect_c(json, ',');
    skip_ws(json);
    v[1] = next_number(json);
    skip_ws(json);
    
    expect_c(json, ',');
    skip_ws(json);
    v[2] = next_number(json);
    skip_ws(json);
    
    expect_c(json, ']');
    return v;
}

// outputs data in buffer to output file
void output_p6(FILE* outputfp, int h, int w) {
    // create header
    fprintf(outputfp, "P6\n%d %d\n%d\n", h, w, MAXCOLOR);
    // writes buffer to output Pixel by Pixel
    fwrite(pixmap, sizeof(Pixel), w*h, outputfp);
}
