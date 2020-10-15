#pragma once
#include <vector>
#include <string>

struct LayoutImage {
    std::string filepath;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct LayoutItem {
    std::string id;
    std::string classname;
    std::string label;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    std::string box;
    std::string down_box;
    int labelfont = 0;
    int labelsize = 14;
    std::string labeltype;
    int textsize = 14;
    int align = 0;
    double value = 0;
    double minimum = 0;
    double maximum = 0;
    double step = 0;
    std::string type;
    std::string callback;
    LayoutImage image;
    bool hidden = false;
    std::string comment;
    std::vector<LayoutItem> items;
};

struct Layout {
    std::vector<LayoutItem> items;
};
