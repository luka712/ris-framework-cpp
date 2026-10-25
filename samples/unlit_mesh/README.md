# unlit_mesh

Loads `assets/cube.obj` / `cube.mtl` (a unit cube with one material per face) into a mesh with one sub mesh per
`usemtl` range, bakes the `Kd` colour of each material into the vertex colours and draws the rotating cube with an
unlit shader and a perspective camera. Port of the `unlit_mesh` example of ris-framework-ts and RisFramework.Demo.
