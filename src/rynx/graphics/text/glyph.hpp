

#pragma once


struct Glyph {
    int id;
    float x;
    float y;
    float width;
    float height;
    float offsetX;
    float offsetY;
    float advanceX;

	Glyph(): id(-1) {}

    Glyph(unsigned char id, float x, float y, float width, float height, float offsetX, float offsetY, float advanceX) {
        this->id = id;
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->offsetX = offsetX;
        this->offsetY = offsetY;
        this->advanceX = advanceX;
    }
};
