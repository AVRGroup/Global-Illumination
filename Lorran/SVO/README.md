# Sparse Voxel Octree (SVO)

Códigos do autor Jeroen Baert (ver arquivo LICENSE).

Há duas aplicações para converter modelos em SVO

* **tri_convert:** Ferramenta para converter uma malha de triângulso (.ply; .obj; etc) para o arquivo .tri, a ser utilizado pela aplicação a seguir.
* **svo_builder:** Out-Of-Core SVO Builder: Particionamento, voxelização e construção da SVO a partir de um arquivo .tri

## Utilização

Caso necessário, compilar através dos arquivos .sh da pasta linux:
```
sh build_tri_convert.sh
```
```
sh build_svo_builder.sh
```

### tri_convert: Convertendo um modelo para formato .tri
Para que se possa construir a SVO, primeiramente é necessário obter o arquivo .tri utilizando a biblioteca libtri já incluida. Isso será feito convertendo o arquivo modelo de entrada (.ply, .off, .3ds, .obj, .sm .ray)

**Sintaxe:** tri_convert -f (caminho para o arquivo do modelo)

**Exemplo**
```
./tri_convert -f ../Bunny/bunny.ply
```
```
tri_convert.exe -f ../Bunny/bunny.ply
```
Isso irá gerar um bunny.tri + bunny.tridata no mesmo diretório.

### svo_builder: Out-Of-Core SVO building
O svo_builder pega um arquivo .tri como entrada e realiza três passos (particionamento, voxelização e construção da SVO).

**Sintaxe:** svo_builder -options

* **-f** (caminho para o arquivo .tri) : Caminho para o arquivo .tri a ser utilizado para construir a SVO (obrigatório).
* **-s** (tamanho do grid) : Resolução do tamanho do grid. Deve ser potência de 2. (Default: 1024)
* **-l** (limite de memória) : Memória limite a ser utilizada, em Mb. (Default: 2048)
* **-d** (porcentagem de quão esparso) : Qual a porcentagem (entre 0.00 e 1.00) de limite de memória que será uilizada para dar speedup na geração da SVO. (Default: 0.10)
* **-levels** Generate intermediare SVO levels' voxel payloads by averaging data from lower levels (which is a quick and dirty way to do low-cost Level-Of-Detail hierarchies). If this option is not specified, only the leaf nodes have an actual payload. (Default: off)
* **-c** (cores) Gera cores para os voxels. Opções: (Default: model)
 * **model** : Dá aos voxels as cores contidas no arquivo .tri. (Será branco caso o modelo original não possua cores)
 * **linear** : Dá aos voxels uma cor RGB linear relacionada à sua posição o grid.
 * **normal** : Obter cores das normais dos triângulos de origem.
 * **fixed** :  Cores fixas dos voxels, configurável no código fonte.
* **-v** Para que seja bastante verbose.

**Exemplos**
```
./svo_builder -f bunny.tri
```
```
svo_builder.exe -f bunny.tri
```
Isso irá gerar um arquivo bunny.octree. Como default, utilizará um grid de dimesão 1024^3, com 2048 Mb de memória do sistema.
```
./svo_builder -f bunny.tri -s 2048 -l 1024 -d 0.2 -c normal -v
```
```
svo_builder.exe -f bunny.tri -s 2048 -l 1024 -d 0.2 -c normal -v
```
Isso irá gerar um arquivo bunny.octree. Como default, utilizará um grid de dimesão 2048^3, com 2048 Mb de memória do sistema, com 20% de speedup de memória adicional. As cores dos voxels serão derivados das normais.
