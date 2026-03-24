typedef struct {
    Vector3 Position;
    Vector3 Size;
    Vector3 Velocity;
    bool Grounded;
    float Yaw;
    float Pitch;
}Player;

Vector3 DefaultPlayerSize = (Vector3){1,1.5,1};

Player SpawnPlayer(Vector3 Pos) {
    Player NewPlayer;
    NewPlayer.Position = Pos;
    NewPlayer.Size = DefaultPlayerSize;
    NewPlayer.Velocity = (Vector3){0,0,0};
    NewPlayer.Grounded = false;
    NewPlayer.Yaw = 0.0f;
    NewPlayer.Pitch = 0.0f;
    return NewPlayer;
}

Vector2 GetChunk(Player player) {
    return (Vector2){floor(player.Position.x / 16), floor(player.Position.z / 16)};
}

Vector2 GetLocalXZ(Player player) {
    return (Vector2){(float)(((int)floor(player.Position.x) % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH), (float)(((int)floor(player.Position.z) % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH)};
}

int BlockAtPos(Vector3 Pos,vector<Chunk>& chunks) {
    Player temp;
    temp.Position = Pos;
    Vector2 GChunk = GetChunk(temp);
    Vector2 Local = GetLocalXZ(temp);
    
    int ChunkIndex = GChunk.y + GChunk.x * WORLD_SIZE;
    int LocalIndex = Local.x + Local.y * CHUNK_WIDTH + (int)floor(Pos.y) * CHUNK_WIDTH * CHUNK_WIDTH;
    
    if(ChunkIndex < 0 || ChunkIndex >= (int)chunks.size()) return 0;
    
    if(Local.x < 0 || Local.x >= CHUNK_WIDTH) return 0;
    if(Local.y < 0 || Local.y >= CHUNK_WIDTH) return 0;
    if(Pos.y < 0 || Pos.y >= CHUNK_HEIGHT) return 0;
    
    Chunk chunk = chunks[ChunkIndex];
    
    return chunk.blocks[LocalIndex];
}

void SetBlockAtPos(Vector3 Pos,vector<Chunk>& chunks,int id) {
    Player temp;
    temp.Position = Pos;
    Vector2 GChunk = GetChunk(temp);
    Vector2 Local = GetLocalXZ(temp);
    
    int ChunkIndex = GChunk.y + GChunk.x * WORLD_SIZE;
    int LocalIndex = Local.x + Local.y * CHUNK_WIDTH + (int)floor(Pos.y) * CHUNK_WIDTH * CHUNK_WIDTH;
    
    if(ChunkIndex < 0 || ChunkIndex >= (int)chunks.size()) return;
    
    if(Local.x < 0 || Local.x >= CHUNK_WIDTH) return;
    if(Local.y < 0 || Local.y >= CHUNK_WIDTH) return;
    if(Pos.y < 0 || Pos.y >= CHUNK_HEIGHT) return;
    
    Chunk& chunk = chunks[ChunkIndex];
    
    chunk.blocks[LocalIndex] = id;
    
    BuildChunkMesh(chunk,false,chunks);
    BuildChunkMesh(chunk,true,chunks);
}

void UpdatePlayerCamera(Camera3D& camera, Player& player, float sensitivity) {
    Vector2 mouseDelta = GetMouseDelta();

    player.Yaw   -= mouseDelta.x * sensitivity;
    player.Pitch -= mouseDelta.y * sensitivity;

    if (player.Pitch >  89.0f * DEG2RAD) player.Pitch =  89.0f * DEG2RAD;
    if (player.Pitch < -89.0f * DEG2RAD) player.Pitch = -89.0f * DEG2RAD;

    Vector3 forward = {
        cosf(player.Pitch) * sinf(player.Yaw),
        sinf(player.Pitch),
        cosf(player.Pitch) * cosf(player.Yaw)
    };

    camera.position = {
        player.Position.x,
        player.Position.y + player.Size.y - 0.2f,
        player.Position.z
    };

    camera.target = Vector3Add(camera.position, forward);
}

void PlayerCollision(Player& player, vector<Chunk>& chunks) {
    float halfX = player.Size.x * 0.5f;
    float halfZ = player.Size.x * 0.5f;

    float yLevels[] = {
        player.Position.y + 0.1f,
        player.Position.y + player.Size.y * 0.5f,
        player.Position.y + player.Size.y - 0.1f
    };

    for (float checkY : yLevels) {
        if (BlockAtPos({player.Position.x + halfX, checkY, player.Position.z}, chunks)) {
            int b = BlockAtPos({player.Position.x + halfX, checkY, player.Position.z}, chunks);
            if (b > 0 && BlockEntries[b].solid)
                player.Position.x = floorf(player.Position.x + halfX) - halfX;
        }
        if (BlockAtPos({player.Position.x - halfX, checkY, player.Position.z}, chunks)) {
            int b = BlockAtPos({player.Position.x - halfX, checkY, player.Position.z}, chunks);
            if (b > 0 && BlockEntries[b].solid)
                player.Position.x = ceilf(player.Position.x - halfX) + halfX;
        }
        if (BlockAtPos({player.Position.x, checkY, player.Position.z + halfZ}, chunks)) {
            int b = BlockAtPos({player.Position.x, checkY, player.Position.z + halfZ}, chunks);
            if (b > 0 && BlockEntries[b].solid)
                player.Position.z = floorf(player.Position.z + halfZ) - halfZ;
        }
        if (BlockAtPos({player.Position.x, checkY, player.Position.z - halfZ}, chunks)) {
            int b = BlockAtPos({player.Position.x, checkY, player.Position.z - halfZ}, chunks);
            if (b > 0 && BlockEntries[b].solid)
                player.Position.z = ceilf(player.Position.z - halfZ) + halfZ;
        }
        if (BlockAtPos({player.Position.x, checkY-0.1f, player.Position.z}, chunks)) {
            int b = BlockAtPos({player.Position.x, player.Position.y - 0.1f, player.Position.z}, chunks);
            if (b > 0 && BlockEntries[b].solid)
                player.Position.y = ceilf(player.Position.y - 0.1f) + 0.1f;
        }
    }
}

Player UpdatePlayer(Player player, vector<Chunk>& chunks) {
    float dt = GetFrameTime();
    int BlockBelowPlayer = BlockAtPos((Vector3){player.Position.x, player.Position.y - player.Size.y + 1, player.Position.z},chunks);
    
    if(BlockBelowPlayer < BlockEntries.size()) if(BlockEntries[BlockBelowPlayer].solid) {
        player.Velocity.y = 0;
        player.Grounded = true;
    } else {
        player.Velocity.y -= 0.98f * dt;
        if(player.Velocity.y < -1.5) player.Velocity.y = -1.5;
        player.Grounded = false;
    }
    
    if(player.Grounded && IsKeyDown(KEY_SPACE)) player.Velocity.y = 0.15f;
    
    Vector3 forward = {
        sinf(player.Yaw),
        0,
        cosf(player.Yaw)
    };
    
    Vector3 right = {
        cosf(player.Yaw),
        0,
        -sinf(player.Yaw)
    };
    
    float speed = 0.16f;
    
    Vector3 movement = {0, 0, 0};
    
    if(IsKeyDown(KEY_W)) movement = Vector3Add(movement, forward);
    if(IsKeyDown(KEY_S)) movement = movement = Vector3Add(movement, Vector3Scale(forward, -1.0f));
    if(IsKeyDown(KEY_D)) movement = Vector3Add(movement, Vector3Scale(right, -1.0f));
    if(IsKeyDown(KEY_A)) movement = Vector3Add(movement, right);
    
    if (Vector3Length(movement) > 0) {
        movement = Vector3Normalize(movement);
        movement = Vector3Scale(movement, speed);
        player.Position = Vector3Add(player.Position, movement);
    }
    
    player.Position = Vector3Add(player.Position,player.Velocity);
    
    if(player.Position.y < -1000) player.Position.y = 1000;
    
    PlayerCollision(player, chunks);
    
    return player;
}