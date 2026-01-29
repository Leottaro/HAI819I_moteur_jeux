#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <fstream>
#include <string>
#include <vector>

// Source courtesy of J. Manson
// http://josiahmanson.com/prose/optimize_ppm/

namespace ppmLoader {
using namespace std;
void eat_comment(ifstream &f);

struct RGB {
    unsigned char r, g, b;
};

struct ImageRGB {
public:
    size_t w, h;
    vector<RGB> data;
};

ImageRGB load_ppm(const string &name);

enum loadedFormat {
    rgb,
    rbg
};

void load_ppm(unsigned char *&pixels, unsigned int &w, unsigned int &h, const string &name, loadedFormat format = rgb);
} // namespace ppmLoader

#endif
