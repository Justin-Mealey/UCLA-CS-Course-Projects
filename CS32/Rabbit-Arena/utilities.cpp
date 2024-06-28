#include <random>
#include "globals.h"
#include "Arena.h"

using namespace std;

// Return a uniformly distributed random int from lowest to highest, inclusive
int randInt(int lowest, int highest)
{
    if (highest < lowest)
        swap(highest, lowest);
    static random_device rd;
    static default_random_engine generator(rd());
    uniform_int_distribution<> distro(lowest, highest);
    return distro(generator);
}
// Return false without changing anything if moving one step from (r,c)
// in the indicated direction would run off the edge of the arena.
// Otherwise, update r and c to the position resulting from the move and
// return true.
bool attemptMove(const Arena& a, int dir, int& r, int& c)
{
    int rnew = r;
    int cnew = c;
    switch (dir)
    {
    case NORTH:  if (r <= 1)        return false; else rnew--; break;
    case EAST:   if (c >= a.cols()) return false; else cnew++; break;
    case SOUTH:  if (r >= a.rows()) return false; else rnew++; break;
    case WEST:   if (c <= 1)        return false; else cnew--; break;
    }
    r = rnew;
    c = cnew;
    return true;
}