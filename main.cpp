//Default C++ Libs
#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <cstdio>
#include <iterator>

using namespace std;

//Raylib Stuffs
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

//OutSource Imports

//InHouse Imports
#include "chunks.hpp"
#include "mesh.hpp"
#include "textures.hpp"
#include "player.hpp"
//#include "rendering.hpp" (Deprecated)

int screenWidth = 800;
int screenHeight = 600;

void PlayerBuild(Player player, vector<Chunk>& chunks, int& block) {
    Vector3 rayforward = {
    cosf(player.Pitch) * sinf(player.Yaw),
    sinf(player.Pitch),
    cosf(player.Pitch) * cosf(player.Yaw)
    };
    
    Vector3 RayPos = (Vector3){
        player.Position.x,
        player.Position.y + player.Size.y - 0.2f,
        player.Position.z
        };
    float RaySpeed = 0.25f;
    
    Vector3 LastPos = RayPos;
    
    DrawText(BlockEntries[block].name.c_str(),20,140,20,WHITE);
    
    if(IsKeyPressed(KEY_ONE)) block++;
    if(IsKeyPressed(KEY_TWO)) block--;
    
    if(block > BlockEntries.size()-1) block = 1;
    if(block < 1) block = BlockEntries.size()-1;
    
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for(int i = 0; i < 10 / RaySpeed; i++) {
            RayPos = Vector3Add(RayPos, Vector3Scale(rayforward, RaySpeed));
            if(BlockAtPos(RayPos,chunks)) {
                SetBlockAtPos(RayPos,chunks,0);
                break;
            }
        }
    }
    
    if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        for(int i = 0; i < 10 / RaySpeed; i++) {
            RayPos = Vector3Add(RayPos, Vector3Scale(rayforward, RaySpeed));
            if(BlockAtPos(RayPos,chunks)) {
                Vector3 LastPos = Vector3Subtract(RayPos, Vector3Scale(rayforward, RaySpeed));;
                SetBlockAtPos(LastPos,chunks,block);
                break;
            } else Vector3 LastPos = RayPos;
        }
    }
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Foundry");
    SetTargetFPS(60);
    SetExitKey(NULL);
    
    NewBlock({}, 0.0f, false, false, false, 0.0f, 0, "Air");
    NewBlock({0}, 0.0f, false, false, true, 0.25f, 0, "Grass");
    NewBlock({1,0}, 0.0f, false, false, true, 0.85f, 0, "Stone");
    NewBlock({2,0}, 0.0f, false, false, true, 0.45f, 0, "Dirt");
    NewBlock({3,0}, 0.0f, false, false, true, 0.55f, 0, "Wood");
    NewBlock({4,0}, 0.0f, true, false, true, 0.35f, 0, "Glass");
    NewBlock({5,0}, 0.15f, true, false, false, 0.0f, 0, "Water");
    NewBlock({6,0}, 0.0f, false, false, true, 0.15f, 0, "Sand");
    NewBlock({7,0}, 0.0f, false, false, true, 0.45f, 0, "Gravel");
    NewBlock({8,9}, 0.0f, false, false, true, 0.45f, 0, "Log");
    NewBlock({10,0}, 0.0f, false, false, true, 0.15f, 0, "Leaf");
    
    LoadPack((char*)"Foundry");
    Texture2D Crosshair = LoadTextureFromPack((char*)"UI/crosshair.png");
    
    vector<Chunk> Chunks;
    InitChunks(Chunks);
    BuildChunkMeshes(Chunks);
    
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 8.0f, 64.0f, 8.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    Player player;
    player = SpawnPlayer((Vector3){ 32.0f, 64.0f, 32.0f });
    
    Material mat = LoadMaterialDefault();
    Material TranslucentChunkMat = LoadMaterialDefault();
    
    Shader TanslucentShader = LoadShader("alphacut.vs", "alphacut.fs");
    
    bool debug = false;
    bool paused = false;
    bool pausedinit = false; 
    bool song = true; 
    
    int selected_block = 1;
    
    InitAudioDevice();
    
    Music Song = LoadSongFromPack((char *)"1.mp3");
    PlayMusicStream(Song);
    while (!WindowShouldClose()) {
        
        if(!paused) {
            if(song) UpdateMusicStream(Song);        
            player = UpdatePlayer(player,Chunks);
            camera.position = (Vector3){player.Position.x, player.Position.y + 1.6f, player.Position.z};
            UpdatePlayerCamera(camera, player, 0.002f);
            SetMousePosition(screenWidth/2,screenHeight/2);
            DisableCursor();
            HideCursor();
            pausedinit = true;
        } else {
            if(pausedinit) {
                ShowCursor();
                EnableCursor();
                pausedinit = false;
            }
        }
        
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        SetWindowSize(GetScreenWidth(), GetScreenHeight());
        
        if(IsKeyPressed(KEY_F3)) debug = !debug;
        if(IsKeyPressed(KEY_ESCAPE)) paused = !paused;
        if(IsKeyPressed(KEY_M)) song = !song;
        
        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);
        
        mat.maps[MATERIAL_MAP_DIFFUSE].texture = GlobalAtlas;
        TranslucentChunkMat.maps[MATERIAL_MAP_DIFFUSE].texture = GlobalAtlas;
        
        
        
        //DrawChunk(chunk);
        for(int x = 0; x < WORLD_SIZE; x++) {
            for(int z = 0; z < WORLD_SIZE; z++) {
                int index = x + z * WORLD_SIZE;
                Chunk chunk = Chunks[index];
                if (chunk.ChunkMesh.vertexCount > 0) DrawMesh(chunk.ChunkMesh, mat, MatrixTranslate(chunk.ChunkX * CHUNK_WIDTH, 0.0f, chunk.ChunkZ * CHUNK_WIDTH));
                TranslucentChunkMat.shader = TanslucentShader;
                if (chunk.TranslucentChunkMesh.vertexCount > 0) DrawMesh(chunk.TranslucentChunkMesh, TranslucentChunkMat, MatrixTranslate(chunk.ChunkX * CHUNK_WIDTH, 0.0f, chunk.ChunkZ * CHUNK_WIDTH));
            }
        }
        EndMode3D();
        
        DrawText("0.1-rx",5,10,20,WHITE);
        
        if(debug) {
            DrawText(TextFormat("XYZ: %.2f, %.2f, %.2f", player.Position.x, player.Position.y, player.Position.z),5,80,20,WHITE);
            DrawText(TextFormat("Local XZ: %.1f, %.1f", fmod(player.Position.x, 16), fmod(player.Position.z, 16)),5,100,20,WHITE);
            DrawText(TextFormat("Chunk XZ: %.0f, %.0f", player.Position.x / 16, player.Position.z / 16),5,120,20,WHITE);
            DrawFPS(5,60);
        }
        
        PlayerBuild(player,Chunks,selected_block);
        
        DrawTextureEx(Crosshair,(Vector2){(float)screenWidth/2-16, (float)screenHeight/2-16}, 0, 2,WHITE);
        
        EndDrawing();
    }   
}