#include "scene.h"

GLFWwindow* StartGLU() {
    if (!glfwInit()) { cerr << "GLFW init failed\n"; return nullptr; }
    GLFWwindow* w = glfwCreateWindow(screenWidth, screenHeight, "Solar System", NULL, NULL);
    if (!w) { cerr << "Window failed\n"; glfwTerminate(); return nullptr; }
    glfwMakeContextCurrent(w);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { cerr << "GLEW failed\n"; glfwTerminate(); return nullptr; }
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return w;
}

GLuint CreateShaderProgram(const char* vs, const char* fs) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, nullptr); glCompileShader(v);
    GLint ok; glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[512]; glGetShaderInfoLog(v,512,0,l); cerr<<"VS:"<<l<<endl; }
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, nullptr); glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[512]; glGetShaderInfoLog(f,512,0,l); cerr<<"FS:"<<l<<endl; }
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char l[512]; glGetProgramInfoLog(p,512,0,l); cerr<<"Link:"<<l<<endl; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void CreateUnitSphere(int sec, int stk) {
    vector<float> verts;
    for (int i = 0; i <= stk; ++i) {
        float sa = M_PI/2 - i*(M_PI/stk), xy = cosf(sa), z = sinf(sa);
        for (int j = 0; j <= sec; ++j) {
            float se = j*(2*M_PI/sec), x = xy*cosf(se), y = xy*sinf(se);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
        }
    }
    vector<GLuint> idx;
    for (int i = 0; i < stk; ++i) {
        int k1 = i*(sec+1), k2 = k1+sec+1;
        for (int j = 0; j < sec; ++j, ++k1, ++k2) {
            if (i) { idx.push_back(k1); idx.push_back(k2); idx.push_back(k1+1); }
            if (i!=stk-1) { idx.push_back(k1+1); idx.push_back(k2); idx.push_back(k2+1); }
        }
    }
    unitSphereVertexCount = idx.size();
    glGenVertexArrays(1, &unitSphereVAO); glGenBuffers(1, &unitSphereVBO);
    glBindVertexArray(unitSphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, unitSphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
    GLuint EBO; glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(GLuint), idx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,0,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,0,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

static void makeStarVAO(GLuint& vao, GLuint& vbo, vector<float>& data) {
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,0,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,0,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

static void addRandomStars(vector<float>& v, int count, float rMin, float rMax) {
    for (int i = 0; i < count; i++) {
        float th = ((float)rand()/RAND_MAX)*2*M_PI;
        float ph = acos(2.0f*((float)rand()/RAND_MAX)-1.0f);
        float r = rMin + (rand()%(int)(rMax-rMin+1));
        float x=r*sin(ph)*cos(th), y=r*sin(ph)*sin(th), z=r*cos(ph);
        v.push_back(x); v.push_back(y); v.push_back(z);
        v.push_back(0); v.push_back(0); v.push_back(1);
    }
}

static void addMilkyWayBand(vector<float>& v, int count) {
    // Dense band of stars tilted ~60 degrees from ecliptic
    float tilt = 1.05f; // radians
    float ct = cos(tilt), st = sin(tilt);
    for (int i = 0; i < count; i++) {
        float lon = ((float)rand()/RAND_MAX) * 2 * M_PI;
        // Gaussian-ish concentration near the band center
        float spread = ((float)rand()/RAND_MAX + (float)rand()/RAND_MAX
                      + (float)rand()/RAND_MAX) / 3.0f - 0.5f;
        float lat = spread * 0.25f; // narrow band
        float r = 48000.0f + (rand() % 8000);
        float x = r * cos(lat) * cos(lon);
        float y = r * cos(lat) * sin(lon);
        float z = r * sin(lat);
        // Rotate around X axis by tilt
        float ry = y * ct - z * st;
        float rz = y * st + z * ct;
        v.push_back(x); v.push_back(ry); v.push_back(rz);
        v.push_back(0); v.push_back(0); v.push_back(1);
    }
}

static StarLayer makeLayer(int count, float ptSize, float r, float g, float b,
                           float rMin, float rMax, bool milkyWay = false) {
    StarLayer sl; sl.pointSize = ptSize;
    sl.r = r; sl.g = g; sl.b = b; sl.a = 1.0f;
    vector<float> data;
    if (milkyWay) addMilkyWayBand(data, count);
    else          addRandomStars(data, count, rMin, rMax);
    sl.count = count;
    makeStarVAO(sl.VAO, sl.VBO, data);
    return sl;
}

void CreateStarfield() {
    srand(42);
    starLayers.clear();
    //                    count  size   R     G     B     rMin    rMax
    // Dim background fill
    starLayers.push_back(makeLayer(5000, 1.0f, 0.45f,0.45f,0.50f, 50000,65000));
    // Medium white (A-type stars)
    starLayers.push_back(makeLayer(2000, 1.4f, 0.82f,0.85f,0.90f, 48000,60000));
    // Blue-white hot (O/B-type) - toned down
    starLayers.push_back(makeLayer( 800, 1.6f, 0.80f,0.85f,1.00f, 45000,58000));
    // Yellow (G-type, Sun-like) - muted
    starLayers.push_back(makeLayer( 700, 1.5f, 1.00f,0.95f,0.80f, 46000,58000));
    // Orange (K-type) - desaturated pastel orange
    starLayers.push_back(makeLayer( 500, 1.3f, 1.00f,0.85f,0.70f, 48000,60000));
    // Red (M-type) - desaturated pastel red/pinkish
    starLayers.push_back(makeLayer( 300, 1.2f, 1.00f,0.75f,0.65f, 50000,62000));
    // Bright featured stars
    starLayers.push_back(makeLayer( 200, 2.8f, 1.00f,0.98f,0.95f, 40000,50000));
    // Milky Way band (dense dim cluster)
    starLayers.push_back(makeLayer(5000, 0.9f, 0.35f,0.33f,0.38f, 0,0, true));
    // Milky Way bright core region
    starLayers.push_back(makeLayer(1500, 1.2f, 0.50f,0.48f,0.52f, 0,0, true));
}

void InitSpacetimeGrid() {
    glGenVertexArrays(1, &spacetimeGridVAO);
    glGenBuffers(1, &spacetimeGridVBO);
    glBindVertexArray(spacetimeGridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, spacetimeGridVBO);
    glVertexAttribPointer(0,3,GL_FLOAT,0,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,0,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

float GetSpacetimeDepth(float x, float z, const vector<Object>& objects) {
    if (physicsMode != EINSTEIN || !showSpacetimeGrid) return 0.0f;
    float maxDepth = 0.0f;
    
    for (size_t k = 0; k < objects.size(); k++) {
        glm::vec3 op = (k==0) ? objects[0].getPosition() : objects[k].getPosition() - objects[0].getPosition();
        glm::vec3 gp = op / (float)SCALE;
        float dx = x - gp.x, dz = z - gp.z;
        
        // Calculate a fixed visual radius for smoothing, always using Enhanced Visual scale (200)
        // so the funnel bowl stays smooth in both Enhanced Visual AND True Scale modes
        float grFixed = (objects[k].getRadius() / (float)SCALE) * 200.0f;
        // The smoothing parameter forces the derivative at the origin to uniquely be 0.
        // This flawlessly rounds the bottom of the singularity spike into a smooth parabolic bowl.
        float smoothing = grFixed * 0.8f; 
        float r = sqrt(dx*dx + dz*dz + smoothing*smoothing);
        
        float massRatio = objects[k].getMass() / 2.0e30f;
        if (massRatio > 1e-8f) {
            // Calibrate the visual Schwarzschild radius to create a perfectly proportioned rubber-sheet funnel
            float rs = 20.0f * massRatio; 
            float R_max = 4000.0f; // Visual flat boundary
            
            if (r < R_max) {
                float r_eff = max(r, rs); // Cap mathematically at Event Horizon
                // The actual Flamm Paraboloid geometry curve: 2 * sqrt(rs * (r - rs))
                float flamm = 2.0f * sqrt(rs * (r_eff - rs));
                float flatY = 2.0f * sqrt(rs * (R_max - rs));
                
                // Deepen the funnel mathematically downward from flat zero
                float y = flamm - flatY; 
                
                // Additive combination of gravity wells
                if (y < maxDepth) maxDepth = y;
            }
        }
    }
    return maxDepth;
}

void UpdateSpacetimeGrid(const vector<Object>& objects) {
    const int R = 150; 
    const float ext = 2000.0f, cell = 2*ext/R;
    
    vector<glm::vec3> pos((R+1)*(R+1));
    for (int iz = 0; iz <= R; iz++) {
        for (int ix = 0; ix <= R; ix++) {
            float x = -ext + ix*cell, z = -ext + iz*cell;
            float y = GetSpacetimeDepth(x, z, objects);
            pos[iz*(R+1)+ix] = glm::vec3(x, y, z);
        }
    }
    
    // Build line pairs
    vector<float> v;
    auto push = [&](glm::vec3 p) {
        v.push_back(p.x); v.push_back(p.y); v.push_back(p.z);
        v.push_back(0); v.push_back(1); v.push_back(0);
    };
    for (int iz = 0; iz <= R; iz++)
        for (int ix = 0; ix < R; ix++) {
            push(pos[iz*(R+1)+ix]); push(pos[iz*(R+1)+ix+1]);
        }
    for (int ix = 0; ix <= R; ix++)
        for (int iz = 0; iz < R; iz++) {
            push(pos[iz*(R+1)+ix]); push(pos[(iz+1)*(R+1)+ix]);
        }
    spacetimeGridVertCount = v.size()/6;
    glBindVertexArray(spacetimeGridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, spacetimeGridVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size()*sizeof(float), v.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
}

void printHelp() {
    cerr << "\n"
    "  +----------------------------------------------------------+\n"
    "  |              SOLAR SYSTEM SIMULATOR                       |\n"
    "  +----------------------------------------------------------+\n"
    "  |  MOVEMENT                                                 |\n"
    "  |    W/S/A/D     Move camera        Space/Ctrl  Up/Down    |\n"
    "  |    Shift       3x speed boost     Scroll/+/-  Zoom       |\n"
    "  |  CAMERA                                                   |\n"
    "  |    F           Toggle free-look   Arrows      Rotate     |\n"
    "  |    R           Reset camera                               |\n"
    "  |  PLANETS                                                  |\n"
    "  |    0-8         Follow planet      `           Cycle next |\n"
    "  |    V           Sun-Focus / Observer view mode             |\n"
    "  |  SIMULATION                                               |\n"
    "  |    P           Pause/Resume       ]/[         Speed +/-  |\n"
    "  |    G           Newton/Einstein    T           Scale mode |\n"
    "  |  DISPLAY                                                  |\n"
    "  |    O           Orbit trails       H           This help  |\n"
    "  |    Escape      Quit                                       |\n"
    "  +----------------------------------------------------------+\n"
    << endl;
}
