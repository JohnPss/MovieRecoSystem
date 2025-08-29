#include "../core/Config.hpp"

#include "../system/FastRecommendationSystem.hpp"
#include "../preprocessing/preProcessament.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        if (process_ratings_file() != 0)
        { 
            return 1; 
        }

        FastRecommendationSystem system;
        system.loadData();
        system.processRecommendations(Config::USERS_FILE);
    }
    catch (const exception &e)
    {
        return 1;
    }

    return 0;
}
