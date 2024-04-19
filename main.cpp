#include <string>
#include <iostream>
#include <vector>
#include <fstream>

#define EPS 1.0e-6

int getNumSubNodesPerElement(int elem_num_type);

int getBoundary2DType(int elem_num_type);

int main() {
	std::string grid_file_path;
	std::string out_file_path;
	bool flag = true;
	std::ifstream infile;
	while (flag) {
		std::cout << "Grid file path : ";
		std::cin >> grid_file_path;
		std::cout << "\n";
		out_file_path = "./Converted/2D_" + grid_file_path + ".msh";
		grid_file_path = "./Original/" + grid_file_path + ".msh";

		infile.open(grid_file_path.c_str());
		if (infile.is_open()) {
			std::cout << "Open Gmsh-type grid file: " + grid_file_path << "\n\n";
			std::cout << "Start demensional reduction..." << std::endl;
			flag = false;
		}			
		else {
			std::cout << "Fail to open Gmsh-type grid file: " + grid_file_path << std::endl;
			std::cout << "Try again..." << std::endl;
		}
	}	

	std::ofstream outfile;
	outfile.open(out_file_path.c_str(), std::ios::trunc);
	outfile.precision(8);

	int num_nodes = 0;
	int num_elems = 0;
	int num_physics = 0;
	int dimension = 3;
	int garbage = 0;

	std::streampos pos_nodes;
	std::streampos pos_elems;
	std::streampos pos_physics;

	std::vector<int> node_index;
	std::vector<int> node_new_index;
	std::vector<bool> node_2Dflag;
	std::vector<double> node_coords;

	int domain_tag = 0;
	std::vector<int> unspecified_tag;

	infile.clear();
	std::string text;
	while (getline(infile, text)) {
		if (text.find("$MeshFormat", 0) != std::string::npos) {
			outfile << text << "\n";
			getline(infile, text);
			outfile << text << "\n";
			getline(infile, text);
			outfile << text << "\n";
		}
		if (text.find("$Nodes", 0) != std::string::npos) {
			outfile << text << "\n";
			infile >> num_nodes;
			pos_nodes = infile.tellg();
			std::cout << "\t -> Number of nodes: " << num_nodes;

			node_index.reserve(num_nodes);
			node_new_index.resize(num_nodes, -1);
			node_coords.resize(num_nodes*dimension, 0);
			node_2Dflag.resize(num_nodes, false);

			int temp_index = 0;
			for (int i = 0; i < num_nodes; i++) {
				infile >> temp_index >> node_coords[i*dimension] >> node_coords[i*dimension + 1] >> node_coords[i*dimension + 2];
				if (node_coords[i*dimension + 2] < EPS) {
					node_2Dflag[i] = true;
					node_index.push_back(temp_index);
					node_new_index[i] = node_index.size();
				}
			}

			outfile << node_index.size() << "\n";
			std::cout << " -> " << node_index.size() << "\n";
			for (int i = 0; i < node_index.size(); i++) {
				int temp_index = node_index[i] - 1;
				outfile << i + 1 << "\t" << node_coords[temp_index*dimension] << "\t" << node_coords[temp_index*dimension + 1] << "\t" << node_coords[temp_index*dimension + 2] << std::endl;
			}
			outfile << "$EndNodes" << "\n";
		}
		if (text.find("$Elements", 0) != std::string::npos) {
			outfile << text << "\n";
			infile >> num_elems;
			pos_elems = infile.tellg();
			std::cout << "\t -> Number of cells and boundaries: " << num_elems;

		}
		if (text.find("$PhysicalNames", 0) != std::string::npos) {
			int bdry_tag = 0;
			std::string bdry_name;
			infile >> num_physics;
			pos_physics = infile.tellg();
			getline(infile, text);
			for (int i = 1; i < num_physics; i++) {
				infile >> garbage >> bdry_tag >> bdry_name;
				if (bdry_name == "\"Domain\"")
					domain_tag = bdry_tag;
				if (bdry_name == "\"Unspecified\"")
					unspecified_tag.push_back(bdry_tag);
			}
		}
	}

	infile.clear();
	infile.seekg(pos_elems);
	getline(infile, text);

	std::vector<int> elem_index(num_elems, 0);
	std::vector<int> elem_type(num_elems, 0);
	std::vector<int> elem_tag(num_elems, 0);
	std::vector<bool> elem_domainflag(num_elems, false);
	std::vector<bool> elem_bdryflag(num_elems, true);
	std::vector<std::vector<int> > elem_nodes(num_elems);

	int bdry_node_index = 0;
	for (int i = 0; i < num_elems; i++) {
		infile >> elem_index[i] >> elem_type[i] >> garbage >> elem_tag[i] >> garbage;
		int num_sub_nodes = getNumSubNodesPerElement(elem_type[i]);
		for (int j = 0; j < num_sub_nodes; j++) {
			infile >> bdry_node_index;
			if (node_2Dflag[bdry_node_index - 1])
				elem_nodes[i].push_back(bdry_node_index);
		}
		if (elem_tag[i] == domain_tag) {
			elem_domainflag[i] = true;
      elem_bdryflag[i] = false;
		}
		for (int j = 0; j < unspecified_tag.size(); j++) {
			if (elem_tag[i] == unspecified_tag[j]) {
				elem_bdryflag[i] = false;
			}
		}
	}

	int ind = 1;
	int num_new_elems = 0;
	std::streampos pos_out_elems = outfile.tellp();
	outfile << num_elems << "\n";
	for (int i = 0; i < num_elems; i++) {
		if (elem_domainflag[i]) {
			outfile << ind++ << "\t" << elem_type[i] << "\t" << "2" << "\t" << elem_tag[i] << "\t" << elem_tag[i];
			for (int j = 0; j < elem_nodes[i].size(); j++) {
				outfile << "\t" << node_new_index[elem_nodes[i][j] - 1];
			}
			outfile << "\n";
		}
		if (elem_bdryflag[i]) {
			int boundary_2d_type = getBoundary2DType(elem_type[i]);
			if (getNumSubNodesPerElement(boundary_2d_type) == elem_nodes[i].size()) {
				outfile << ind++ << "\t" << boundary_2d_type << "\t" << "2" << "\t" << elem_tag[i] << "\t" << elem_tag[i];
				for (int j = 0; j < elem_nodes[i].size(); j++) {
					outfile << "\t" << node_new_index[elem_nodes[i][j] - 1];
				}
				outfile << "\n";
			}
		}
	}
	num_new_elems = ind - 1;
	outfile << "$EndElements\n";
	outfile << "$PhysicalNames\n";
	outfile << num_physics << "\n";
	
	infile.clear();
	infile.seekg(pos_physics);
	getline(infile, text);
	std::string bdry_name;
	for (int i = 0; i < num_physics; i++) {
		infile >> garbage; outfile << garbage << "\t";
		infile >> garbage; outfile << garbage << "\t";
		infile >> bdry_name; outfile << bdry_name << "\t";
		outfile << "\n";
	}
	outfile << "$EndPhysicsNames\n";

	outfile.clear();
	outfile.seekp(pos_out_elems);
	outfile << num_new_elems << std::endl;

	std::cout << " -> " << num_new_elems << "\n\n";
	std::cout << "Conversion complete!!! " << "\n";
	std::cout << "Converted file path: " << out_file_path << "\n";

	infile.close();
	outfile.close();

	return 0;
}

enum class GmshElemType {
	LINE_P1 = 1, LINE_P2 = 8, LINE_P3 = 26, LINE_P4 = 27, LINE_P5 = 28, LINE_P6 = 62,
	TRIS_P1 = 2, TRIS_P2 = 9, TRIS_P3 = 21, TRIS_P4 = 23, TRIS_P5 = 25,
	QUAD_P1 = 3, QUAD_P2 = 10, QUAD_P3 = 36, QUAD_P4 = 37, QUAD_P5 = 38, QUAD_P6 = 47,
	TETS_P1 = 4, TETS_P2 = 11, TETS_P3 = 29, TETS_P4 = 30, TETS_P5 = 31,
	HEXA_P1 = 5, HEXA_P2 = 12, HEXA_P3 = 92, HEXA_P4 = 93, HEXA_P5 = 94,
	PRIS_P1 = 6, PRIS_P2 = 13, PRIS_P3 = 90, PRIS_P4 = 91, PRIS_P5 = 106,
	PYRA_P1 = 7, PYRA_P2 = 14, PYRA_P3 = 118, PYRA_P4 = 119
};

int getNumSubNodesPerElement(const int elem_num_type)
{
	switch (static_cast<GmshElemType>(elem_num_type)) {
	case GmshElemType::LINE_P1: return 2;
	case GmshElemType::LINE_P2: return 3;
	case GmshElemType::LINE_P3: return 4;
	case GmshElemType::LINE_P4: return 5;
	case GmshElemType::LINE_P5: return 6;
	case GmshElemType::LINE_P6: return 7;
	case GmshElemType::TRIS_P1: return 3;
	case GmshElemType::TRIS_P2: return 6;
	case GmshElemType::TRIS_P3: return 10;
	case GmshElemType::TRIS_P4: return 15;
	case GmshElemType::TRIS_P5: return 21;
	case GmshElemType::QUAD_P1: return 4;
	case GmshElemType::QUAD_P2: return 9;
	case GmshElemType::QUAD_P3: return 16;
	case GmshElemType::QUAD_P4: return 25;
	case GmshElemType::QUAD_P5: return 36;
	case GmshElemType::QUAD_P6: return 49;
	case GmshElemType::TETS_P1: return 4;
	case GmshElemType::TETS_P2: return 10;
	case GmshElemType::TETS_P3: return 20;
	case GmshElemType::TETS_P4: return 35;
	case GmshElemType::TETS_P5: return 56;
	case GmshElemType::HEXA_P1: return 8;
	case GmshElemType::HEXA_P2: return 27;
	case GmshElemType::HEXA_P3: return 64;
	case GmshElemType::HEXA_P4: return 125;
	case GmshElemType::HEXA_P5: return 216;
	case GmshElemType::PRIS_P1: return 6;
	case GmshElemType::PRIS_P2: return 18;
	case GmshElemType::PRIS_P3: return 40;
	case GmshElemType::PRIS_P4: return 75;
	case GmshElemType::PRIS_P5: return 126;
	case GmshElemType::PYRA_P1: return 5;
	case GmshElemType::PYRA_P2: return 14;
	case GmshElemType::PYRA_P3: return 30;
	case GmshElemType::PYRA_P4: return 55;
	default:
		std::cout << "Gmsh-element type number("  << elem_num_type << ") is not supported" << std::endl;
	}
	return -1;
}

int getBoundary2DType(int elem_num_type)
{
	switch (static_cast<GmshElemType>(elem_num_type)) {
	case GmshElemType::TRIS_P1: return static_cast<int>(GmshElemType::LINE_P1);
	case GmshElemType::TRIS_P2: return static_cast<int>(GmshElemType::LINE_P2);
	case GmshElemType::TRIS_P3: return static_cast<int>(GmshElemType::LINE_P3);
	case GmshElemType::TRIS_P4: return static_cast<int>(GmshElemType::LINE_P4);
	case GmshElemType::TRIS_P5: return static_cast<int>(GmshElemType::LINE_P5);
	case GmshElemType::QUAD_P1: return static_cast<int>(GmshElemType::LINE_P1);
	case GmshElemType::QUAD_P2: return static_cast<int>(GmshElemType::LINE_P2);
	case GmshElemType::QUAD_P3: return static_cast<int>(GmshElemType::LINE_P3);
	case GmshElemType::QUAD_P4: return static_cast<int>(GmshElemType::LINE_P4);
	case GmshElemType::QUAD_P5: return static_cast<int>(GmshElemType::LINE_P5);
	case GmshElemType::QUAD_P6: return static_cast<int>(GmshElemType::LINE_P6);
	default:
		std::cout << "Converting Gmsh-element type number(" << elem_num_type << ") is not supported" << std::endl;
	}
	return -1;
}