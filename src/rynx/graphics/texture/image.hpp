
#pragma once

#include <string>
#include <memory>

struct Image {
	Image() {}
	~Image() {
		if(data)
			unload();
	}
	
	void loadImage(const std::string& filename);
	void unload();
	
	unsigned sizeX = 0;
	unsigned sizeY = 0;
	char* data = nullptr;
	bool hasAlpha = false;
};

