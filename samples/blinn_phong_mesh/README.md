# blinn_phong_mesh

Loads `assets/cube.obj` / `cube.mtl` into a mesh with one sub mesh per `usemtl` range and draws it with Blinn-Phong
materials (`Kd`, `Ka` falling back to `Kd`, `Ks`, `Ns`), a pulsing ambient light, one directional light and two
coloured point lights that orbit the cube. Port of the `blinn_phong_mesh` example of ris-framework-ts and
RisFramework.Demo. Lights are plain uniforms, there is no scene class in 0.1.0.
