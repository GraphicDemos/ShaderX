!!ARBfp1.0
#
# Shader: Bumpy reflections
# Author: Henning Mortveit
# Version: h = infinity shader
#
# Assumption: Model matrix is identity such that all coordinates are
# world coordinates.
#####################################################################
#
# Variables:
#
# Varying
#
# - Texture coord for base texture and normal map in texcoord[0]
# - World position of fragment in texcoord[1]
#
# Uniforms:
#
# - Te = S.PM.VM'     : program.local[0..3]  matrix for reflection (texgen)
# - ratio [dae/(h+dae), dae, 0.0, 0.0];
# - view point E      : program.local[5]
# - object dist h     : program.local[6] .x
# - World tangent fT  : program.local[7] .xyz   
# - World bitangent fB: program.local[8] .xyz
# - World normal fN   : program.local[9] .xyz (redundant)
# - Reflected E       : program.local[10]
#
# Texture:
# 
# - baseTexture       : texture[0]
# - normalMap         : texture[1]
# - reflectionTexture : texture[2]
#
###########################################################################
#
# Setup:
#
# Varying:
#
ATTRIB texCoord = fragment.texcoord[0]; 
ATTRIB x        = fragment.texcoord[1]; 
#
# Uniform: 
#
PARAM Te[4]     = { program.local[0..3] }; 
PARAM ratio     = program.local[4];     # [dae/(dae+h), dae, 0, 0]
PARAM E         = program.local[5];     # viewpoint
PARAM h         = program.local[6];     # distance to object
PARAM fT        = program.local[7];     # tangent 
PARAM fB        = program.local[8];     # bitangent
PARAM fN        = program.local[9];     # normal 
PARAM Er        = program.local[10];    # reflected viewpoint
#
##############################################################

TEMP r0, r1; 

# Load local normal from normal map and transform [0,1] to [-1,1].
# Finish by transforming to world coordinates

TXP r0.xyz, texCoord, texture[1], 2D;
MAD r0.xyz, r0, 2.0, -1.0; 

TEMP N1;
MUL N1.xyz, r0.x, fT;
MAD N1.xyz, r0.y, fB, N1;
MAD N1.xyz, r0.z, fN, N1;

RCP r0.w, x.w;
MUL r0, x, r0.w;                    # frag. world coord. w. w = 1 ==>r0

SUB r1.xyz, E, r0;                  # View vector V = E - X ==> r1

TEMP VN1;                           # VN1: reflection of V about N1;
DP3 r0.w, r1, N1;
MUL r0.w, r0.w, 2.0;
MAD VN1.xyz, r0.w, N1, -r1;

# Offset is Er + dae/<N,VN1>)VN1. ==> r0

DP3 r0.w, fN, VN1;
RCP r0.w, r0.w;
MUL r0.w, r0.w, ratio.y;
MAD r0, r0.w, VN1, Er;
MOV r0.w, 1.0;

DP4 r1.x, Te[0], r0;                 # TexGen for reflection texture. 
DP4 r1.y, Te[1], r0;
DP4 r1.z, Te[2], r0;
DP4 r1.w, Te[3], r0;

TXP r0, r1, texture[2], 2D;          # Reflection texture lookup.
TEX r1, texCoord, texture[0], 2D;    # base texture lookup
LRP r0, 0.7, r0, r1;                 # blend

# Put in some bump mapped diffuse lighting. 
PARAM lv = {0.57735, 0.57735, 0.57735, 0.0};
DP3 r1, lv, N1;

MUL result.color, r1, r0;

END 
