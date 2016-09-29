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

#define CAMERA 0
#define SPHERE 1
#define PLANE 2

typedef struct {
    int kind;
    union {
        struct {
            double width;
            double height;
        } camera;
        struct {
            double color[3];
            double position[3];
            double radius;
        } sphere;
        struct {
            double color[3];
            double position[3];
            double normal[3];
        } plane;
    };
} Object;

int line = 1;
Object** objects;

void read_scene(char*);
void skip_ws(FILE*);
void expect_c(FILE*, int);
int next_c(FILE*);
char* next_string(FILE*);
double next_number(FILE*);
double* next_vector(FILE*);

/*
 * 
 */
int main(int argc, char** argv) {
    objects = malloc(sizeof(Object*)*129);
    read_scene("test.json");
    
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
                objects[i]->kind = CAMERA;
            } else if (strcmp(value, "sphere") == 0) {
                objects[i]->kind = SPHERE;
            } else if (strcmp(value, "plane") == 0) {
                objects[i]->kind = PLANE;
            } else {
                fprintf(stderr, "Error: Unknown type, '%s', on line number %d.\n", value, line);
                exit(1);
            }
            
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
                    
                    if ((strcmp(key, "width") == 0) || 
                        (strcmp(key, "height") == 0) || 
                        (strcmp(key, "radius") == 0)) {
                        double value = next_number(json);
                        if (objects[i]->kind == CAMERA) {
                            if (strcmp(key, "width") == 0)
                                objects[i]->camera.width = value;
                            else if (strcmp(key, "height") == 0)
                                objects[i]->camera.height = value;
                            else
                                fprintf(stderr, "Error: Unknown property '%s' for 'camera' on line number %d.\n", key, line);
                        } else if (objects[i]->kind == SPHERE) {
                            if (strcmp(key, "radius") == 0) {
                                objects[i]->sphere.radius = value;
                            } else {
                                fprintf(stderr, "Error: Unknown property '%s' for 'sphere' on line number %d.\n", key, line);
                            }
                        } else if (objects[i]->kind == PLANE) {
                            fprintf(stderr, "Error: Unknown property '%s' for 'plane' on line number %d.\n", key, line);
                        }
                    } else if ((strcmp(key, "color") == 0) || 
                               (strcmp(key, "position") == 0) || 
                               (strcmp(key, "normal") == 0)) {
                        double* value = next_vector(json);
                        if (objects[i]->kind == CAMERA) {
                            fprintf(stderr, "Error: Unknown property '%s' for 'camera' on line number %d.\n", key, line);
                        } else if (objects[i]->kind == SPHERE) {
                            if (strcmp(key, "color") == 0) {
                                objects[i]->sphere.color[0] = value[0];
                                objects[i]->sphere.color[1] = value[1];
                                objects[i]->sphere.color[2] = value[2];
                            } else if (strcmp(key, "position") == 0) {
                                objects[i]->sphere.position[0] = value[0];
                                objects[i]->sphere.position[1] = value[1];
                                objects[i]->sphere.position[2] = value[2];
                            } else {
                                fprintf(stderr, "Error: Unknown property '%s' for 'sphere' on line number %d.\n", key, line);
                            }
                        } else if (objects[i]->kind == PLANE) {
                            if (strcmp(key, "color") == 0) {
                                objects[i]->plane.color[0] = value[0];
                                objects[i]->plane.color[1] = value[1];
                                objects[i]->plane.color[2] = value[2];
                            } else if (strcmp(key, "position") == 0) {
                                objects[i]->plane.position[0] = value[0];
                                objects[i]->plane.position[1] = value[1];
                                objects[i]->plane.position[2] = value[2];
                            } else if (strcmp(key, "normal") == 0) {
                                objects[i]->plane.normal[0] = value[0];
                                objects[i]->plane.normal[1] = value[1];
                                objects[i]->plane.normal[2] = value[2];
                            } else {
                                fprintf(stderr, "Error: Unknown property '%s' for 'plane' on line number %d.\n", key, line);
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
