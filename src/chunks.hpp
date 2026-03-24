#include "blocks.hpp"

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 64

#ifdef PLATFORM_WEB
    #define WORLD_SIZE 8
#else
    #define WORLD_SIZE 16
#endif

typedef struct {
    vector<int> blocks;
    Mesh ChunkMesh;
    Mesh TranslucentChunkMesh;
    bool ChunkUpdated;
    int ChunkX;
    int ChunkZ;
} Chunk;

typedef struct {
    bool hit;
    Vector3 point;
    Vector3 normal;
    int blockX, blockY, blockZ;
    int face;
    float distance;
} RaycastHit;

bool IsFaceCovered(const Chunk& chunk, int x, int y, int z, int face, const vector<Chunk>& chunks) {
    int otherX = x;
    int otherY = y;
    int otherZ = z;

    switch (face) {
        case 1: otherZ++; break;
        case 2: otherZ--; break;
        case 3: otherX++; break;
        case 4: otherX--; break;
        case 5: otherY++; break;
        case 6: otherY--; break;
        default: return false;
    }

    if (otherY < 0 || otherY >= CHUNK_HEIGHT) return false;

    int neighborCX = chunk.ChunkX;
    int neighborCZ = chunk.ChunkZ;

    if (otherX < 0) {
        neighborCX -= 1;
        otherX = CHUNK_WIDTH - 1;
    } else if (otherX >= CHUNK_WIDTH) {
        neighborCX += 1;
        otherX = 0;
    }

    if (otherZ < 0) {
        neighborCZ -= 1;
        otherZ = CHUNK_WIDTH - 1;
    } else if (otherZ >= CHUNK_WIDTH) {
        neighborCZ += 1;
        otherZ = 0;
    }

    const Chunk* targetChunk = &chunk;
    if (neighborCX != chunk.ChunkX || neighborCZ != chunk.ChunkZ) {
        targetChunk = nullptr;
        for (int i = 0; i < (int)chunks.size(); i++) {
            if (chunks[i].ChunkX == neighborCX && chunks[i].ChunkZ == neighborCZ) {
                targetChunk = &chunks[i];
                break;
            }
        }
        if (targetChunk == nullptr) return false;
    }

    int Index = x + z * CHUNK_WIDTH + y * CHUNK_WIDTH * CHUNK_WIDTH;
    int otherIndex = otherX + otherZ * CHUNK_WIDTH + otherY * CHUNK_WIDTH * CHUNK_WIDTH;

    int blockId2 = chunk.blocks[Index];
    int blockId = targetChunk->blocks[otherIndex];

    if (blockId <= 0 || blockId >= (int)BlockEntries.size()) return false;

    BlockEntry& entry = BlockEntries[blockId];

    if (entry.translucent || entry.transparency > 0) {
        if (blockId == blockId2) return true;
        return false;
    }

    return true;
}

bool IsBlockCovered(const Chunk& chunk, int x, int y, int z, const vector<Chunk>& chunks) {
    for (int face = 1; face <= 6; face++) {
        if (!IsFaceCovered(chunk, x, y, z, face, chunks)) {
            return false;
        }
    }
    return true;
}

int GetBlockIndex(int x, int y, int z) {
    return x + z * CHUNK_WIDTH + y * CHUNK_WIDTH * CHUNK_WIDTH;
}

bool IsValidBlockCoord(int x, int y, int z) {
    return x >= 0 && x < CHUNK_WIDTH &&
           y >= 0 && y < CHUNK_HEIGHT &&
           z >= 0 && z < CHUNK_WIDTH;
}

bool IsBlockSolid(const Chunk& chunk, int x, int y, int z) {
    if (!IsValidBlockCoord(x, y, z)) return false;
    return chunk.blocks[GetBlockIndex(x, y, z)] != 0;
}

Chunk InitChunk(int cx, int cz) {
    Chunk chunk;
    chunk.ChunkMesh = {0};
    chunk.TranslucentChunkMesh = {0};
    chunk.ChunkUpdated = false;
    
    chunk.blocks.resize(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH);
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {

                int index = x +
                            z * CHUNK_WIDTH +
                            y * CHUNK_WIDTH * CHUNK_WIDTH;
                
                float fx = (float)(x + cx * CHUNK_WIDTH);
                float fz = (float)(z + cz * CHUNK_WIDTH);
                
                float ground_height_f = 32.0f 
                    + 3.0f * sin(fx * 0.036) 
                    + 6.0f * sin(fx * 0.0015f) * cos(fz * 0.12f)
                    + 2.0f * sin((fx + fz) * 0.0036f);
                
                int ground_height = (int)ground_height_f;

                if (z > 4 && z < 16 && y > 28 && y <ground_height+1 && cz == 0)  chunk.blocks[index] = 7; 
                    else if (z < 5 && z > -1 && y > ground_height-2 && y <ground_height+1 && cz == 0) { if(ground_height == y) chunk.blocks[index] = 6; }
                    else if (y == ground_height)  chunk.blocks[index] = 1; 
                    else if (y < ground_height-4)  chunk.blocks[index] = 2; 
                    else if (y < ground_height)  chunk.blocks[index] = 3;

            }
        }
    }
    
    
    //Trees
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {

                int index = x +
                            z * CHUNK_WIDTH +
                            y * CHUNK_WIDTH * CHUNK_WIDTH;
                
                float fx = (float)(x + cx * CHUNK_WIDTH);
                float fz = (float)(z + cz * CHUNK_WIDTH);
                
                float ground_height_f = 32.0f 
                    + 3.0f * sin(fx * 0.036) 
                    + 6.0f * sin(fx * 0.0015f) * cos(fz * 0.12f)
                    + 2.0f * sin((fx + fz) * 0.0036f);
                
                int ground_height = (int)ground_height_f;
                
                if(y >=ground_height && y <=ground_height +4 && z % 6 == 0 && cx % 5 == 0 && cz % 3 ==0 && x %13 == 0) {
                    chunk.blocks[index] = 9;
                    if( y == ground_height+4) for(int ix = -2; ix < 2; ix++) {
                        for(int iy = 1; iy < 3; iy++) {
                            for(int iz = -2; iz < 2; iz++) {
                                int index = (x+ix) +
                                (z+iz) * CHUNK_WIDTH +
                                (y+iy) * CHUNK_WIDTH * CHUNK_WIDTH;
                                chunk.blocks[index] = 10;
                            }
                        }
                    }
                }
            }
        }
    }
    
    chunk.ChunkX = cx;
    chunk.ChunkZ = cz;
    
    return chunk;
}

void InitChunks(vector<Chunk>& chunks) {
    for(int x = 0; x < WORLD_SIZE; x++) {
            for(int z = 0; z < WORLD_SIZE; z++) {
                int index = x + z * WORLD_SIZE;
                chunks.push_back(InitChunk(x, z));
        }
    }
}