#ifndef PREPROCESSAMENTO_HPP
#define PREPROCESSAMENTO_HPP

#include "Config.hpp"

struct Rating {
    int movieId;
    float rating;
    
    Rating(int id, float r) : movieId(id), rating(r) {}
    
    bool operator<(const Rating& other) const {
        if (movieId != other.movieId) {
            return movieId < other.movieId;
        }
        return rating < other.rating;
    }
};

struct DataChunk {
    char* start;
    char* end;
    std::unordered_map<int, std::vector<Rating>> local_user_data;
    std::unordered_map<int, int> local_movie_count;
};

inline bool is_digit(char c);
inline int safe_fast_stoi(char*& p, char* end);
inline float safe_fast_stof(char*& p, char* end);
inline void safe_advance_to_next_line(char*& p, char* end);



void process_chunk(DataChunk* chunk);

void filter_and_write_chunk(const DataChunk* chunk, const std::unordered_set<int>* valid_movies, int thread_id);

void concatenate_temp_files(int num_threads);

int process_ratings_file();

const char* find_ratings_file();

#endif 