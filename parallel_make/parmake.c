/**
 * parallel_make
 * CS 241 - Spring 2022
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "stdio.h"
#include "unistd.h"
#include "queue.h"
#include "sys/stat.h"
#include "time.h"
#include "pthread.h"

graph* dependency_graph = NULL;
queue* q = NULL;
size_t queue_size = 0;
dictionary * nodes = NULL;
vector * rules = NULL;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
// pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

int find_cycle(dictionary * history, void * node);
int is_cyclic(void * node);
void update_dictionary(dictionary *dict, void *key, int new_value);
void fill_queue(vector*);
void* make(void*);  
void get_rules(vector *);

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    nodes = string_to_int_dictionary_create();
    dependency_graph = parser_parse_makefile(makefile, targets);
    if(num_threads < 1) {return 0;}
    vector *target_vector= graph_neighbors(dependency_graph, "");
    size_t num_targets = vector_size(target_vector);
    // look for cycles
    bool cycle_found = false;
    for (size_t i = 0; i < num_targets; ++i) {
        void *curr = vector_get(target_vector, i);
        if (is_cyclic(curr) == 1) {
            print_cycle_failure((char*) curr);
            cycle_found = true;
        }
    }
    if(!cycle_found) {
        rules = shallow_vector_create();
        get_rules(target_vector);
        pthread_t threads[num_threads];
        for(size_t i = 0; i < num_threads; i++) {
            if(pthread_create(&threads[i], NULL, make, NULL) != 0) {
                exit(1);
            }
        }

        for(size_t i = 0; i < num_threads; i++) {
            if(pthread_join(threads[i], NULL) != 0) {
                exit(1);
            }
        }
        vector_destroy(rules);
    }
    graph_destroy(dependency_graph);
    vector_destroy(target_vector);
    dictionary_destroy(nodes);
    return 0;
}

int is_cyclic(void * node) {
    if(dependency_graph == NULL) {
        return -1;
    }
    if(!graph_contains_vertex(dependency_graph, node)) {
        return -1;
    }
    dictionary *history = string_to_int_dictionary_create();
    vector *keys = graph_vertices(dependency_graph);
    size_t num_keys = vector_size(keys);
    for (size_t i = 0; i < num_keys; ++i) {
        int value = 0;
        dictionary_set(history, vector_get(keys, i), &value);
    }
    int result = find_cycle(history, node);
    dictionary_destroy(history);
    vector_destroy(keys);
    return result;
    
}

int find_cycle(dictionary * history, void * node) {
    int *state = dictionary_get(history, node);
    // target is in progress, which means cycle is found
    if (*state == 1)
        return 1;
    // target is done   
    if (*state == 2)
        return 0;
    // set state to "in progress"
    update_dictionary(history, node, 1);
    // check all of its descendants
    vector *neighbors = graph_neighbors(dependency_graph, node);
    size_t num_neighbors = vector_size(neighbors);
    for (size_t i = 0; i < num_neighbors; ++i) {
        if (find_cycle(history, vector_get(neighbors, i)) == 1) {
            vector_destroy(neighbors);
            return 1;
        }
    }
    // set state to "finished"
    update_dictionary(history, node, 2);
    vector_destroy(neighbors);
    return 0;
}

void update_dictionary(dictionary *dict, void *key, int new_value) {
    int *value = dictionary_get(dict, key);
    *value = new_value;
}

void fill_queue(vector * target_vector) {
    if(vector_size(target_vector) == 0) {
        vector_destroy(target_vector);
        return;
    }

    for(size_t i = 0; i < vector_size(target_vector); i++) {
        // fprintf(stderr, "%s", (char*)vector_get(target_vector, i));
        if(!dictionary_contains(nodes,(char*)vector_get(target_vector, i))) {
            dictionary_set(nodes, (char*)vector_get(target_vector, i), NULL);
            queue_push(q, vector_get(target_vector, i));
            queue_size++;
        }
        fill_queue(graph_neighbors(dependency_graph, vector_get(target_vector, i)));
        
    }
    vector_destroy(target_vector);
}

int get_run_value(void * target) {
    rule_t *rule = (rule_t*)graph_get_vertex_value(dependency_graph, target);
    if(rule->state != 0) {
        return 3;
    }
    vector * neighbors = graph_neighbors(dependency_graph, target);
    size_t num_neighbors = vector_size(neighbors);
    if(num_neighbors > 0) {
        if(access(target, F_OK) != -1) {
            for (size_t i = 0; i < num_neighbors; ++i) {
                char *sub_vertex = vector_get(neighbors, i);
                if (access(sub_vertex, F_OK) != -1) {
                    struct stat stat_0, stat_1;
                    if (stat((char *)target, &stat_0) == -1 || stat(sub_vertex, &stat_1) == -1) {
                        vector_destroy(neighbors);
                        return -1;
                    }   
                    if (difftime(stat_0.st_mtime, stat_1.st_mtime) < 0) {
                        vector_destroy(neighbors);
                        return 1; 
                    }
                } else {
                    vector_destroy(neighbors);
                    return 1;
                } 
            }
            vector_destroy(neighbors);
            return 2;   
        } else {
            pthread_mutex_lock(&mutex2);
            for(size_t i = 0; i < num_neighbors; i++) {
                rule_t* rule = (rule_t*)graph_get_vertex_value(dependency_graph, vector_get(neighbors, i));
                int x = rule->state;
                if(x != 1) {
                    pthread_mutex_unlock(&mutex2);
                    vector_destroy(neighbors);
                    return x;
                }
            }
            pthread_mutex_unlock(&mutex2);
            vector_destroy(neighbors);
            return 1;
        }
    } else {
        vector_destroy(neighbors);
        return access(target, F_OK) != -1 ? 2 : 1;
    }
}


void * make(void * goal) {
    goal = goal;
    while(1) {
        pthread_mutex_lock(&mutex1);
        size_t num_rules = vector_size(rules);
        // fprintf(stderr, "%ld\n", num_rules);
        if(num_rules > 0) {
            for(size_t i = 0; i < num_rules; i++) {
                void * target = vector_get(rules, i);
                int status = get_run_value(target);
                rule_t *rule = (rule_t*)graph_get_vertex_value(dependency_graph, target);
                // fprintf(stderr, "%d\n", status);
                if(status == 1) {
                    vector_erase(rules, i);
                    pthread_mutex_unlock(&mutex1);
                    vector * commands = rule->commands;
                    size_t num_commands = vector_size(commands);
                    int after_command_state = 1;
                    for(size_t i = 0; i < num_commands; i++) {
                        if(system((char*)vector_get(commands, i)) != 0) {
                            after_command_state = -1;
                            break;
                        }
                    }
                    pthread_mutex_lock(&mutex2);
                    rule->state = after_command_state;
                    pthread_cond_broadcast(&cond1);
                    pthread_mutex_unlock(&mutex2);
                    break;
                } else if (status == -1 || status == 2) {
                    vector_erase(rules, i);
                    pthread_mutex_unlock(&mutex1);
                    pthread_mutex_lock(&mutex2);
                    rule->state = status == -1 ? -1 : 1;
                    pthread_cond_broadcast(&cond1);
                    pthread_mutex_unlock(&mutex2);
                    break;
                } else if (status == 3) {
                    vector_erase(rules, i);
                    pthread_mutex_unlock(&mutex1);
                    break;
                } else if (i == num_rules - 1) {
                    pthread_cond_wait(&cond1, &mutex1);
                    pthread_mutex_unlock(&mutex1);
                    break;
                }
            }
        } else {
            pthread_mutex_unlock(&mutex1);
            return NULL;
        }
    }
}
void get_rules_helper(vector*, vector*, dictionary*);
void get_rules(vector* target_vector) {
    dictionary * d = string_to_int_dictionary_create();
    vector * vertices = graph_vertices(dependency_graph);
    size_t num_vertices = vector_size(vertices);
    // fprintf(stderr, "%ld\n", num_vertices);
    // for(size_t i = 0; i < num_vertices; i++) {
    //     // fprintf(stderr, "%s ", (char*)vector_get(vertices, i));
    // }
    for(size_t i = 0; i < num_vertices; i++) {
        int x = 1;
        dictionary_set(d, vector_get(vertices, i), &x);
    }
    get_rules_helper(rules, target_vector, d);
    vector_destroy(vertices);
    dictionary_destroy(d);
}

void get_rules_helper(vector * rules, vector * target_vector, dictionary * d) {
    size_t num_targets = vector_size(target_vector);
    for(size_t i = 0; i < num_targets; i++) {
        void * target = vector_get(target_vector, i);
        
        vector * neighbors = graph_neighbors(dependency_graph, target);
        get_rules_helper(rules, neighbors, d);
        // fprintf(stderr, "%d:\n", *((int*)dictionary_get(d, target)));
        if(*((int*)dictionary_get(d, target)) == 1) {
            // fprintf(stderr, "%d\n", 1);
            update_dictionary(d, target, 0);
            // fprintf(stderr, "%s ", (char*)target);
            vector_push_back(rules, target);
        }
        vector_destroy(neighbors);
    }
}