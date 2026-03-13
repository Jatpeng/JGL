#include "tinyxml2.h"
#include "material.h"

int main() {
	tinyxml2::XMLDocument doc;
	doc.LoadFile("example.xml");
	tinyxml2::XMLElement* root = doc.FirstChildElement("Material");
	Material material;
	material.name = root->Attribute("Name");

	for (tinyxml2::XMLElement* child = root->FirstChildElement("Param"); child != NULL; child = child->NextSiblingElement("Param")) {
		Param param;
		param.name = child->Attribute("Name");
		param.type = child->Attribute("Type");
		param.defaultValue = child->Attribute("Default");
		material.params.push_back(param);
	}

	material.print();

	return 0;
}