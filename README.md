# Description
Dimensional reduction from 3D high-order Gmsh format to 2D high-order Gmsh format

# Manual
1. Generate 3D mesh in Pointwise by extrude of 2D mesh
2. Set CAE>Select Solver...>Gmsh
3. Set target 2D plane at z=0
4. Assign boundary condition name as "Domain" for the taget 2D plane
5. Leave useless boundaries as "Unassigned"
6. Generate High order 3D Gmsh file
7. Put the Gmsh file together with the code
8. Run and type the Gmsh file name
