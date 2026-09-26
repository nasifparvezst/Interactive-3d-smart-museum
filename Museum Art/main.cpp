#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

// ================================================================
// Interactive 3D Bangladesh Liberation War Museum
// CSE 444 Computer Graphics Project
// ---------------------------------------------------------------
// Features implemented:
// - Two-storey 3D Bangladesh Liberation War Museum + exterior entrance
// - Human-height first-person WASD + mouse camera with wall collision
// - Object translation, rotation and scaling
// - Step 9: redesigned natural-color tiger, elephant and spotted deer + cleaned benches
// - Step 10A: walkable grand staircase + true second floor shell, landing and railings
// - Step 10B: one large open-plan Level 2 Liberation War Exhibition (1971)
// - Step 11: widened safe staircase/landings and readable black-on-ivory exhibit labels
// - Step 12: farther exterior start, doorway-overlap removal, redesigned dual-face illuminated clock,
//            high-contrast plinth names, two additional Level-2 feature sculptures, and stronger wildlife colors/shapes
// - Step 17: startup-safe Windows/freeglut material-state recovery
// - Step 14: refined tiger/deer facial detail, lighter wildlife tessellation, outdoor ticket counter, visitor info totem and bench
// - Step 18: Level 2 converted to Liberation War Exhibition (1971) with map, archival-style scenes, weapons, documents and field-equipment cases
// - Step 13: Smart Museum interaction layer: proximity-based exhibit detection, E-key information cards,
//            room-aware visitor prompts, and non-cluttering digital interpretation for major exhibits
// - 8-source layered museum lighting (directional, point and gallery spotlights)
// - Ambient, diffuse and specular materials
// - Decorative marble floor inlays, room borders and corridor accents
// - Coffered ceiling details, recessed fixtures and wall wainscoting
// - Procedural archival-style wall panels, maps and themed displays
// - Step 8 interaction/layout pass: reliable transform controls, overlap audit, high-contrast illuminated entrance clock
// - Museum-style non-occluding glass-frame artifact cases with varied exhibits
// - Reduced wall-side seating, flush wall mounting and unobstructed visitor circulation
// - Portrait-ready classical gallery frames for future real/user-supplied photos
// - Continuously rotating kinetic chandelier and ceiling fans
// - Dense museum pass: artillery cannon, 1971 military jeep, mortar/crates, command-post dioramas, wall artifact racks and larger visitor population
// - Real Level-2 window openings with visible exterior landscape
// - Different camera modes
// ================================================================

constexpr float PI = 3.14159265358979323846f;

struct Vec3 {
    float x, y, z;
    Vec3(float X=0, float Y=0, float Z=0) : x(X), y(Y), z(Z) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
    Vec3 operator*(float s) const { return Vec3(x*s, y*s, z*s); }
};

static float lengthVec(const Vec3& v) {
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

static Vec3 normalizeVec(const Vec3& v) {
    float L = lengthVec(v);
    if (L < 0.0001f) return Vec3(0,0,0);
    return Vec3(v.x/L, v.y/L, v.z/L);
}

static float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

// ---------------------------- Window -----------------------------
int gWinW = 1280;
int gWinH = 720;

// ---------------------------- Camera -----------------------------
Vec3 cameraPos(0.0f, 2.15f, 63.0f);
float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
bool overviewCamera = false;
bool mouseLookEnabled = true;
bool firstMouse = true;
int lastMouseX = 0;
int lastMouseY = 0;

bool keyState[256] = {false};
float moveSpeed = 4.8f;
float mouseSensitivity = 0.16f;

// ---------------------------- Scene ------------------------------
float centralRotation = 0.0f;
float fanRotation = 0.0f;
bool animateScene = true;
bool showHelp = true;
bool lightsOn[3] = {true, true, true};

struct ObjectTransform {
    float tx=0.0f, ty=0.0f, tz=0.0f;
    float rotY=0.0f;
    float scale=1.0f;
};

// Four transformable ground-floor Liberation War memorial highlights.
ObjectTransform artXform[4];
int selectedArt = 0;

// ---------------------- Smart museum layer -----------------------
// A real museum usually keeps the room uncluttered and gives visitors
// interpretation near an exhibit.  Step 13 does that digitally: when the
// visitor approaches a major object, the HUD offers an E-key information card.
bool showExhibitInfo = false;
int exhibitInfoId = -1;

struct ExhibitInfo {
    const char* title;
    const char* collection;
    const char* line1;
    const char* line2;
    float x, z;
    bool upperFloor;
};

// Entries 0-3 follow the actual transformable first-floor memorial locations.
const ExhibitInfo exhibitInfoTable[7] = {
    {"7 MARCH SPEECH MEMORIAL","BANGLADESH LIBERATION WAR MUSEUM",
     "A commemorative sculpture anchors the first-floor opening narrative.",
     "The surrounding gallery introduces the movement and public events of 1971.",
     -8.65f,22.0f,false},
    {"FREEDOM FIGHTER GROUP MEMORIAL","BANGLADESH LIBERATION WAR MUSEUM",
     "A group memorial represents the freedom fighters of the Liberation War.",
     "Nearby displays present field equipment, communication and resistance material.",
      8.65f,22.0f,false},
    {"CIVILIAN MEMORY MEMORIAL","BANGLADESH LIBERATION WAR MUSEUM",
     "A memorial sculpture marks the civilian experience of the war.",
     "Archive and relief displays nearby interpret displacement and wartime care.",
     -8.65f,-20.4f,false},
    {"VICTORY & INDEPENDENCE MEMORIAL","BANGLADESH LIBERATION WAR MUSEUM",
     "A flag-centered memorial closes the first-floor historical journey.",
     "The rear wall combines remembrance material with the story of victory.",
      0.0f,-31.0f,false},
    {"BANGLADESH 1971 MAP","LIBERATION WAR EXHIBITION",
     "A schematic Bangladesh map is used to orient visitors inside the 1971 exhibition.",
     "The map marks the eleven Liberation War sectors as an educational overview.",
       0.0f, 35.0f, true},
    {"WAR ARTIFACTS","LIBERATION WAR EXHIBITION",
     "Historical-display models include rifles, field radio, helmets and medical equipment.",
     "The objects are shown as museum artifacts, not as functional equipment.",
      -10.0f, 17.0f, true},
    {"VICTORY & MEMORY","LIBERATION WAR EXHIBITION",
     "The rear section combines archival panels, documents and a victory-memory display.",
     "This floor completes the museum journey with remembrance and historical context.",
       0.0f,-14.0f, true}
};

// ----------------------- Two-floor building ----------------------
// Ground floor is Y=0 and is curated as the road-to-independence exhibition.
// The upper-floor slab sits above it as the main 1971 war exhibition.
constexpr float GROUND_FLOOR_Y = 0.0f;
constexpr float SECOND_FLOOR_Y = 9.90f;
constexpr float EYE_HEIGHT = 1.72f;
constexpr float STAIR_MID_Y = SECOND_FLOOR_Y * 0.5f;

// Grand staircase occupies the front-right side of the lobby.
constexpr float STAIR_Z_FRONT = 33.55f;
constexpr float STAIR_Z_REAR  = 26.55f;
constexpr float STAIR_FIRST_X = 17.15f;
constexpr float STAIR_SECOND_X = 21.00f;
constexpr float STAIR_FLIGHT_W = 2.75f;

// Step 11: generous turning/arrival zones so a first-person visitor does not
// fall from the stairs while turning at the mid landing or stepping onto Level 2.
constexpr float STAIR_LANDING_X_MIN = 15.55f;
constexpr float STAIR_LANDING_X_MAX = 23.35f;
constexpr float STAIR_LANDING_Z_MIN = 25.05f;
constexpr float STAIR_LANDING_Z_MAX = 28.70f;
constexpr float STAIR_TOP_Z_MIN = 32.70f;
constexpr float STAIR_TOP_Z_MAX = 35.45f;

// --------------------------- Textures -----------------------------
GLuint texFloor = 0;
GLuint texWall = 0;
GLuint texWood = 0;
GLuint texCeiling = 0;
GLuint texPainting1 = 0;
GLuint texPainting2 = 0;
GLuint texPainting3 = 0;
GLuint texPainting4 = 0;
GLuint texPainting5 = 0;
GLuint texPainting6 = 0;
GLuint texPainting7 = 0;
GLuint texPainting8 = 0;
GLuint texPainting9 = 0;
GLuint texPainting10 = 0;
GLuint texPainting11 = 0; // calligraphy-inspired cultural art
GLuint texPainting12 = 0; // geometric heritage motif
GLuint texPainting13 = 0; // floral manuscript motif
GLuint texPainting14 = 0; // warm folk-art composition
GLuint texPortrait1 = 0;  // stylized portrait placeholder 1
GLuint texPortrait2 = 0;  // stylized portrait placeholder 2
GLuint texPortrait3 = 0;  // stylized portrait placeholder 3
GLuint texVillage = 0;    // Bangladesh village landscape artwork
GLuint texWarMap = 0;     // Optimized Bangladesh map for 1971 exhibition
GLuint texWarMapDetail = 0; // Compact Liberation War sector / route map
GLuint texWarPhoto1 = 0;
GLuint texWarPhoto2 = 0;
GLuint texWarPhoto3 = 0;
GLuint texWarPhoto4 = 0;
GLuint texWarPhoto5 = 0;
GLuint texWarPhoto6 = 0;
GLuint texWarPhoto7 = 0;
GLuint texWarPhoto8 = 0;
GLuint texCarpet = 0;

// ================================================================
// Utility: material and primitive drawing
// ================================================================

void setMaterial(float r, float g, float b, float shininess=32.0f,
                 float spec=0.35f, float ambientFactor=0.25f) {
    // Keep glColor and material synchronized. This is robust on older/freeglut
    // fixed-function setups where GL_COLOR_MATERIAL is enabled.
    glColor4f(r,g,b,1.0f);
    GLfloat ambient[]  = {r*ambientFactor, g*ambientFactor, b*ambientFactor, 1.0f};
    GLfloat diffuse[]  = {r, g, b, 1.0f};
    GLfloat specular[] = {spec, spec, spec, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void setMaterialAlpha(float r, float g, float b, float a,
                      float shininess=64.0f, float spec=0.8f) {
    glColor4f(r,g,b,a);
    GLfloat ambient[]  = {r*0.18f, g*0.18f, b*0.18f, a};
    GLfloat diffuse[]  = {r, g, b, a};
    GLfloat specular[] = {spec, spec, spec, a};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void drawUnitTexturedCube(GLuint texture) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1,1,1);

    glBegin(GL_QUADS);
    // Front +Z
    glNormal3f(0,0,1);
    glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f, 0.5f);
    glTexCoord2f(1,0); glVertex3f( 0.5f,-0.5f, 0.5f);
    glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f, 0.5f);
    glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f, 0.5f);

    // Back -Z
    glNormal3f(0,0,-1);
    glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f,-0.5f);
    glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f,-0.5f);
    glTexCoord2f(1,1); glVertex3f(-0.5f, 0.5f,-0.5f);
    glTexCoord2f(0,1); glVertex3f( 0.5f, 0.5f,-0.5f);

    // Left -X
    glNormal3f(-1,0,0);
    glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f,-0.5f);
    glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f, 0.5f);
    glTexCoord2f(1,1); glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f,-0.5f);

    // Right +X
    glNormal3f(1,0,0);
    glTexCoord2f(0,0); glVertex3f(0.5f,-0.5f, 0.5f);
    glTexCoord2f(1,0); glVertex3f(0.5f,-0.5f,-0.5f);
    glTexCoord2f(1,1); glVertex3f(0.5f, 0.5f,-0.5f);
    glTexCoord2f(0,1); glVertex3f(0.5f, 0.5f, 0.5f);

    // Top +Y
    glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-0.5f,0.5f, 0.5f);
    glTexCoord2f(1,0); glVertex3f( 0.5f,0.5f, 0.5f);
    glTexCoord2f(1,1); glVertex3f( 0.5f,0.5f,-0.5f);
    glTexCoord2f(0,1); glVertex3f(-0.5f,0.5f,-0.5f);

    // Bottom -Y
    glNormal3f(0,-1,0);
    glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f,-0.5f);
    glTexCoord2f(1,0); glVertex3f( 0.5f,-0.5f,-0.5f);
    glTexCoord2f(1,1); glVertex3f( 0.5f,-0.5f, 0.5f);
    glTexCoord2f(0,1); glVertex3f(-0.5f,-0.5f, 0.5f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void drawBox(float x, float y, float z, float sx, float sy, float sz,
             float r, float g, float b, float shininess=32.0f, float spec=0.3f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterial(r,g,b,shininess,spec);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawTexturedBox(float x, float y, float z, float sx, float sy, float sz,
                     GLuint tex, float shininess=18.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterial(1,1,1,shininess,0.18f);
    drawUnitTexturedCube(tex);
    glPopMatrix();
}

void drawSphere(float x,float y,float z,float radius,
                float r,float g,float b,float shininess=32,float spec=0.35f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    setMaterial(r,g,b,shininess,spec);
    glutSolidSphere(radius, 24, 18);
    glPopMatrix();
}

void drawScaledSphere(float x,float y,float z,
                      float sx,float sy,float sz,
                      float r,float g,float b,float shininess=32,float spec=0.35f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterial(r,g,b,shininess,spec);
    glutSolidSphere(1.0, 26, 20);
    glPopMatrix();
}

void drawCylinderY(float x, float y, float z, float radius, float height,
                  float r, float g, float b, int slices=24, float spec=0.25f) {
    if (radius <= 0.0f || height <= 0.0f) return;

    glPushMatrix();
    glTranslatef(x, y, z);
    setMaterial(r,g,b,24,spec);

    // Main cylinder: base at (x,y,z), extending upward along +Y.
    static GLUquadric* q = nullptr;
    if (!q) {
        q = gluNewQuadric();
        gluQuadricNormals(q, GLU_SMOOTH);
    }

    gluCylinder(q, radius, radius, height,
                 std::max(8, slices), 1);

    // Small caps keep the model visually solid and prevent see-through ends.
    glPushMatrix();
    glRotatef(180.0f, 1, 0, 0);
    gluDisk(q, 0.0, radius, std::max(8, slices), 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, height, 0);
    gluDisk(q, 0.0, radius, std::max(8, slices), 1);
    glPopMatrix();

    glPopMatrix();
}

void drawCylinderBetween(const Vec3& a, const Vec3& b, float radius,
                         float r,float g,float bl) {
    Vec3 d = b-a;
    float L = lengthVec(d);
    if (L < 0.001f) return;
    Vec3 n = normalizeVec(d);

    float ax = std::acos(clampf(n.y,-1.0f,1.0f)) * 180.0f/PI;
    Vec3 axis(n.z, 0.0f, -n.x);
    float axisLen = lengthVec(axis);

    glPushMatrix();
    glTranslatef(a.x,a.y,a.z);
    if (axisLen > 0.0001f) glRotatef(ax, axis.x,axis.y,axis.z);
    else if (n.y < 0) glRotatef(180,1,0,0);

    static GLUquadric* q = nullptr;
    if (!q) {
        q = gluNewQuadric();
        gluQuadricNormals(q, GLU_SMOOTH);
    }
    setMaterial(r,g,bl,24,0.25f);
    glRotatef(-90,1,0,0);
    gluCylinder(q,radius,radius,L,16,2);
    glPopMatrix();
}

void drawConeY(float x,float y,float z,float base,float height,
               float r,float g,float b) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(-90,1,0,0);
    setMaterial(r,g,b,24,0.25f);
    glutSolidCone(base,height,20,8);
    glPopMatrix();
}

// ================================================================
// Procedural texture generation
// ================================================================

GLuint uploadTexture(const std::vector<unsigned char>& data, int w, int h) {
    GLuint id;
    glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_2D,id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w,h, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return id;
}

GLuint makeMarbleTexture(int w=128,int h=128) {
    std::vector<unsigned char> d(w*h*3);
    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            float v = 0.5f + 0.5f*std::sin(x*0.13f + std::sin(y*0.09f)*3.0f);
            int base = 195 + int(35*v);
            int i=(y*w+x)*3;
            d[i+0]=(unsigned char)clampf(base+7,0,255);
            d[i+1]=(unsigned char)clampf(base+8,0,255);
            d[i+2]=(unsigned char)clampf(base+12,0,255);
        }
    }
    return uploadTexture(d,w,h);
}

GLuint makeWallTexture(int w=128,int h=128) {
    std::vector<unsigned char> d(w*h*3);
    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            bool seam = (x%32==0 || y%24==0);
            int i=(y*w+x)*3;
            if (seam) { d[i]=185; d[i+1]=181; d[i+2]=175; }
            else {
                unsigned char n=(unsigned char)(225 + ((x*17+y*11)%11));
                d[i]=n; d[i+1]=(unsigned char)(n-2); d[i+2]=(unsigned char)(n-3);
            }
        }
    }
    return uploadTexture(d,w,h);
}

GLuint makeWoodTexture(int w=128,int h=128) {
    std::vector<unsigned char> d(w*h*3);
    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            float grain = 0.5f + 0.5f*std::sin(y*0.45f + std::sin(x*0.05f)*2.0f);
            int i=(y*w+x)*3;
            d[i]=(unsigned char)(95 + 50*grain);
            d[i+1]=(unsigned char)(52 + 26*grain);
            d[i+2]=(unsigned char)(25 + 15*grain);
        }
    }
    return uploadTexture(d,w,h);
}

GLuint makeCeilingTexture(int w=64,int h=64) {
    std::vector<unsigned char> d(w*h*3);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        int i=(y*w+x)*3;
        bool line=(x%16==0 || y%16==0);
        unsigned char c=line?210:242;
        d[i]=c; d[i+1]=c; d[i+2]=(unsigned char)std::min(255,int(c)+3);
    }
    return uploadTexture(d,w,h);
}

GLuint makeCarpetTexture(int w=128,int h=128) {
    std::vector<unsigned char> d(w*h*3);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        int i=(y*w+x)*3;
        bool border=(x<6 || x>w-7 || y<6 || y>h-7);
        bool motif=((x/16 + y/16)%2==0);
        if(border){ d[i]=180; d[i+1]=140; d[i+2]=45; }
        else if(motif){ d[i]=93; d[i+1]=24; d[i+2]=35; }
        else { d[i]=68; d[i+1]=15; d[i+2]=25; }
    }
    return uploadTexture(d,w,h);
}

GLuint makePaintingTexture(int type,int w=128,int h=128) {
    std::vector<unsigned char> d(w*h*3);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        float u=float(x)/(w-1), v=float(y)/(h-1);
        int i=(y*w+x)*3;
        unsigned char R=0,G=0,B=0;
        if(type==0){ // sunset mountains
            R=(unsigned char)(220-80*v); G=(unsigned char)(120+80*(1-v)); B=(unsigned char)(80+120*(1-v));
            float mountain = 0.62f + 0.12f*std::sin(u*18.0f);
            if(v>mountain){R=45;G=48;B=62;}
            float dx=u-0.72f, dy=v-0.25f;
            if(dx*dx+dy*dy<0.012f){R=255;G=220;B=110;}
        } else if(type==1){ // abstract waves
            float q=0.5f+0.5f*std::sin(u*20+v*12);
            R=(unsigned char)(40+190*q); G=(unsigned char)(60+130*(1-q)); B=(unsigned char)(110+120*v);
        } else if(type==2){ // forest
            R=(unsigned char)(40+40*v); G=(unsigned char)(85+100*(1-v)); B=(unsigned char)(60+55*(1-v));
            if(((x/13)%3)==0 && y>35){R=80;G=48;B=28;}
        } else if(type==3){ // geometric modern art
            bool a=((x/20+y/20)%2==0);
            bool c=((x-64)*(x-64)+(y-64)*(y-64)<900);
            if(c){R=230;G=182;B=50;}
            else if(a){R=35;G=95;B=155;}
            else {R=180;G=58;B=75;}
        } else if(type==4){ // ocean / sky
            float horizon=0.46f;
            if(v<horizon){ R=(unsigned char)(85+70*(1-v)); G=(unsigned char)(145+60*(1-v)); B=(unsigned char)(205+40*(1-v)); }
            else { R=(unsigned char)(20+20*v); G=(unsigned char)(80+55*v); B=(unsigned char)(125+80*(1-v)); }
            if(v>0.52f && ((x+y)%27)<2){ R=220;G=235;B=238; }
        } else if(type==5){ // night city
            R=18; G=26; B=(unsigned char)(48+40*(1-v));
            int bw=12+(x%31); int top=45+(x*7)%38;
            if(y>top && ((x/11)%2==0)){ R=35;G=42;B=58; }
            if(y>top && (x%9==2) && (y%12<5)){ R=245;G=205;B=92; }
            float dx=u-0.75f,dy=v-0.20f;
            if(dx*dx+dy*dy<0.008f){R=235;G=235;B=205;}
            (void)bw;
        } else if(type==6){ // botanical
            R=232;G=225;B=200;
            float cx=64, cy=68;
            float ang=std::atan2(float(y)-cy,float(x)-cx);
            float rad=std::sqrt((x-cx)*(x-cx)+(y-cy)*(y-cy));
            if(rad<46 && std::fmod(std::fabs(ang)*18.0f+rad*0.18f,7.0f)<2.6f){R=48;G=122;B=72;}
            if(rad<18){R=196;G=116;B=74;}
        } else if(type==7){ // tiger-inspired portrait
            R=220;G=128;B=35;
            bool stripe = (std::sin(x*0.22f + std::sin(y*0.10f)*1.7f) > 0.62f);
            if(stripe){R=35;G=25;B=18;}
            int dx=x-64, dy=y-58;
            if((dx*dx)/(34*34.0f)+(dy*dy)/(28*28.0f)<1.0f){
                if(!stripe){R=232;G=151;B=46;}
                if((x-52)*(x-52)+(y-54)*(y-54)<18 || (x-76)*(x-76)+(y-54)*(y-54)<18){R=235;G=224;B=170;}
                if((x-52)*(x-52)+(y-54)*(y-54)<5 || (x-76)*(x-76)+(y-54)*(y-54)<5){R=15;G=12;B=10;}
                if((x-64)*(x-64)+(y-68)*(y-68)<34){R=245;G=224;B=178;}
            }
        } else if(type==8){ // desert / heritage
            R=(unsigned char)(210+25*(1-v)); G=(unsigned char)(150+40*(1-v)); B=(unsigned char)(85+30*(1-v));
            float dune=0.58f+0.08f*std::sin(u*12.0f);
            if(v>dune){R=176;G=113;B=58;}
            if(x>52 && x<72 && y>38 && y<90){R=95;G=66;B=43;}
            if(x>46 && x<78 && y>36 && y<43){R=120;G=82;B=50;}
        } else if(type==9){ // monochrome museum poster
            int c=190 + int(45*std::sin((x+y)*0.08f));
            R=G=B=(unsigned char)clampf(c,0,255);
            if(((x/18)+(y/18))%3==0){R=62;G=74;B=88;}
            if((x-64)*(x-64)+(y-64)*(y-64)<420){R=172;G=65;B=58;}
        } else if(type==10){ // calligraphy-inspired parchment
            R=236; G=222; B=184;
            // warm aged-paper variation
            int grain=((x*19+y*13)%17)-8;
            R=(unsigned char)clampf(int(R)+grain,0,255);
            G=(unsigned char)clampf(int(G)+grain,0,255);
            B=(unsigned char)clampf(int(B)+grain/2,0,255);
            // dark flowing ornamental strokes (abstract, not a real script)
            float yy=float(y);
            float c1=58.0f + 12.0f*std::sin(x*0.085f);
            float c2=78.0f + 10.0f*std::sin(x*0.11f+1.6f);
            float c3=42.0f + 8.0f*std::sin(x*0.15f+0.4f);
            if(std::fabs(yy-c1)<2.2f || (x>22 && x<111 && std::fabs(yy-c2)<1.7f) ||
               (x>10 && x<88 && std::fabs(yy-c3)<1.4f)){ R=42;G=31;B=21; }
            // vertical flourish and gold medallion
            if(x>57 && x<63 && y>28 && y<102){R=54;G=38;B=23;}
            int dx=x-92,dy=y-34;
            if(dx*dx+dy*dy<160){R=182;G=136;B=42;}
            if(x<5 || x>w-6 || y<5 || y>h-6){R=126;G=88;B=28;}
        } else if(type==11){ // geometric heritage / mosaic
            R=26;G=65;B=82;
            int cx=(x%32)-16, cy=(y%32)-16;
            int d=std::abs(cx)+std::abs(cy);
            if(d<6){R=226;G=178;B=63;}
            else if(d<10){R=181;G=67;B=55;}
            else if(((x/16)+(y/16))%2==0){R=39;G=111;B=110;}
            if((x%32<2)||(y%32<2)){R=224;G=207;B=154;}
        } else if(type==12){ // floral manuscript
            R=230;G=216;B=178;
            float cx=64.0f,cy=64.0f;
            float dx=x-cx,dy=y-cy;
            float rad=std::sqrt(dx*dx+dy*dy);
            float ang=std::atan2(dy,dx);
            if(rad<43 && std::fmod(std::fabs(ang)*25.0f+rad*0.28f,9.0f)<2.2f){R=44;G=110;B=69;}
            for(int k=0;k<8;k++){
                float a=k*PI/4.0f;
                float px=cx+31.0f*std::cos(a), py=cy+31.0f*std::sin(a);
                float ex=x-px,ey=y-py;
                if(ex*ex+ey*ey<72){R=178;G=57;B=67;}
            }
            if(rad<9){R=211;G=154;B=45;}
            if(x<6 || x>w-7 || y<6 || y>h-7){R=74;G=117;B=86;}
        } else if(type==13){ // warm Bengal folk-art inspired composition
            R=223;G=167;B=72;
            bool checker=((x/24+y/24)%2==0);
            if(checker){R=190;G=69;B=50;}
            float wave=64.0f+20.0f*std::sin(x*0.08f);
            if(std::fabs(y-wave)<5){R=33;G=83;B=75;}
            int dx=x-64,dy=y-64;
            if(dx*dx+dy*dy<420){R=232;G=207;B=128;}
            if(dx*dx+dy*dy<160){R=62;G=71;B=68;}
            if(x<5 || x>w-6 || y<5 || y>h-6){R=61;G=43;B=31;}
        } else if(type>=14 && type<=16){ // stylized portrait placeholders
            const int variant=type-14;
            // Neutral museum backdrop with a vignette. These are intentionally
            // generic placeholders and can later be replaced by real photos.
            if(variant==0){ R=205; G=193; B=170; }
            if(variant==1){ R=176; G=190; B=196; }
            if(variant==2){ R=202; G=184; B=176; }
            float dx=float(x)-64.0f, dy=float(y)-57.0f;
            float vign=clampf((dx*dx+dy*dy)/5200.0f,0.0f,1.0f);
            R=(unsigned char)clampf(int(R)-int(38*vign),0,255);
            G=(unsigned char)clampf(int(G)-int(38*vign),0,255);
            B=(unsigned char)clampf(int(B)-int(38*vign),0,255);
            // shoulders
            float sx=(float(x)-64.0f)/38.0f, sy=(float(y)-100.0f)/26.0f;
            if(sx*sx+sy*sy<1.0f){ R=58+variant*18; G=54+variant*13; B=51+variant*17; }
            // neck
            if(x>56 && x<72 && y>72 && y<95){ R=188;G=150;B=120; }
            // head / face
            float hx=(float(x)-64.0f)/21.0f, hy=(float(y)-55.0f)/27.0f;
            if(hx*hx+hy*hy<1.0f){ R=196;G=158;B=128; }
            // hair / silhouette
            float hair=(float(x)-64.0f)*(float(x)-64.0f)/520.0f + (float(y)-45.0f)*(float(y)-45.0f)/360.0f;
            if(hair<1.0f && y<57){ R=46;G=38;B=34; }
            // eyes and simple facial features
            if(((x-56)*(x-56)+(y-54)*(y-54)<7) || ((x-72)*(x-72)+(y-54)*(y-54)<7)){ R=35;G=30;B=28; }
            if(x>61 && x<67 && y>58 && y<70){ R=160;G=118;B=96; }
            if(x>56 && x<72 && y>72 && y<75){ R=112;G=62;B=58; }
            // thin inner border
            if(x<5 || x>w-6 || y<5 || y>h-6){ R=86;G=68;B=45; }
        } else {
            R=180;G=180;B=180;
        }
        d[i]=R;d[i+1]=G;d[i+2]=B;
    }
    return uploadTexture(d,w,h);
}

GLuint makeVillageTexture(int w=160,int h=128) {
    std::vector<unsigned char> d(w*h*3);
    auto put=[&](int x,int y,unsigned char R,unsigned char G,unsigned char B){
        if(x<0||x>=w||y<0||y>=h) return;
        int i=(y*w+x)*3; d[i]=R; d[i+1]=G; d[i+2]=B;
    };
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        float v=float(y)/(h-1);
        unsigned char R,G,B;
        if(v<0.48f){ // warm rural sky
            R=(unsigned char)(112+85*(1-v/0.48f));
            G=(unsigned char)(175+55*(1-v/0.48f));
            B=(unsigned char)(214+32*(1-v/0.48f));
        } else if(v<0.63f){ // distant green tree line
            R=62; G=112; B=55;
        } else if(v<0.80f){ // rice field
            float wave=0.5f+0.5f*std::sin(x*0.12f+y*0.045f);
            R=(unsigned char)(78+35*wave); G=(unsigned char)(132+55*wave); B=(unsigned char)(43+18*wave);
        } else { // foreground path/earth
            R=154; G=116; B=67;
        }
        put(x,y,R,G,B);
    }
    // River band
    for(int y=79;y<94;y++) for(int x=0;x<w;x++) {
        float yy=y+5.0f*std::sin(x*0.075f);
        if(std::fabs(y-yy)<7){
            unsigned char R=57,G=139,B=177;
            if((x+y)%17<2){R=180;G=210;B=214;}
            put(x,y,R,G,B);
        }
    }
    // Bamboo/large trees
    for(int x0 : {18,145}) {
        for(int y=39;y<84;y++) for(int t=-2;t<=2;t++) put(x0+t,y,44,83,35);
        for(int yy=29;yy<58;yy++) for(int xx=-18;xx<=18;xx++) {
            float dx=xx/18.0f, dy=(yy-43)/15.0f;
            if(dx*dx+dy*dy<1.0f) put(x0+xx,yy,40,103,43);
        }
    }
    // Traditional village hut with tin/straw roof
    for(int y=57;y<83;y++) for(int x=63;x<111;x++) put(x,y,194,139,76);
    for(int y=50;y<62;y++) {
        float left=59+(y-50)*0.35f, right=115-(y-50)*0.35f;
        for(int x=(int)left;x<=(int)right;x++) put(x,y,105,61,32);
    }
    for(int y=63;y<78;y++) for(int x=81;x<91;x++) put(x,y,71,47,29); // door
    for(int y=67;y<74;y++) for(int x=99;x<107;x++) put(x,y,88,155,178); // window
    // Small footpath toward hut
    for(int y=83;y<113;y++) {
        int half=3+(y-83)/4;
        int cx=87+(int)(4*std::sin(y*0.16f));
        for(int x=cx-half;x<=cx+half;x++) put(x,y,181,143,88);
    }
    // Coconut/palm tree
    for(int y=39;y<76;y++) put(130+(int)(0.08f*(y-39)),y,72,53,30);
    for(int a=0;a<7;a++){
        float ang=-1.1f+a*0.36f;
        for(int k=0;k<30;k++){
            int x=133+(int)(std::cos(ang)*k*0.55f);
            int y=39+(int)(std::sin(ang)*k*0.28f);
            put(x,y,31,105,47);
            if(k%3==0) put(x+1,y,44,128,52);
        }
    }
    // Birds
    for(int bx: {36,47,118}){
        int by=31+(bx%3)*4;
        for(int k=-4;k<=4;k++){ put(bx+k,by+std::abs(k)/2,32,42,45); }
    }
    return uploadTexture(d,w,h);
}


// Historical-display textures are intentionally archival-style illustrations.
// They keep the project self-contained while avoiding external image-file dependencies.
GLuint makeLiberationPhotoTexture(int type,int w=160,int h=110) {
    std::vector<unsigned char> d(w*h*3);
    auto put=[&](int x,int y,int R,int G,int B){
        if(x<0||x>=w||y<0||y>=h) return;
        int i=(y*w+x)*3;
        d[i]=(unsigned char)clampf(float(R),0,255);
        d[i+1]=(unsigned char)clampf(float(G),0,255);
        d[i+2]=(unsigned char)clampf(float(B),0,255);
    };
    auto rect=[&](int x0,int y0,int x1,int y1,int R,int G,int B){
        for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) put(x,y,R,G,B);
    };
    auto circle=[&](int cx,int cy,int r,int R,int G,int B){
        for(int y=cy-r;y<=cy+r;y++) for(int x=cx-r;x<=cx+r;x++)
            if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r) put(x,y,R,G,B);
    };
    auto line=[&](int x0,int y0,int x1,int y1,int R,int G,int B,int thick=1){
        int dx=std::abs(x1-x0), dy=std::abs(y1-y0), steps=std::max(dx,dy);
        if(steps==0){ put(x0,y0,R,G,B); return; }
        for(int k=0;k<=steps;k++){
            float t=float(k)/steps;
            int x=int(std::round(x0+(x1-x0)*t));
            int y=int(std::round(y0+(y1-y0)*t));
            for(int oy=-thick;oy<=thick;oy++) for(int ox=-thick;ox<=thick;ox++)
                put(x+ox,y+oy,R,G,B);
        }
    };

    // aged monochrome / sepia photographic field
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        float v=float(y)/float(h-1);
        int grain=((x*17+y*13)%19)-9;
        int base=204-int(52*v)+grain;
        put(x,y,base+5,base+1,base-4);
    }
    rect(5,5,w-6,h-6,192,186,170);

    if(type==0){ // mass movement / public gathering
        rect(8,8,w-9,72,182,176,163);
        for(int i=0;i<18;i++){
            int x=14+i*8+(i%3);
            int ground=78-(i%4)*3;
            circle(x,ground-15-(i%3),4,49,46,43);
            rect(x-3,ground-11,x+3,ground,63,59,54);
            line(x-2,ground-4,x-9-(i%2)*3,ground-19,61,57,52,1);
            line(x+2,ground-4,x+8+(i%2)*3,ground-20,61,57,52,1);
        }
        // green/red flag silhouette
        line(126,22,126,72,55,54,46,2);
        for(int y=25;y<46;y++) for(int x=127;x<151;x++){
            int R=34,G=78,B=58;
            if((x-136)*(x-136)+(y-35)*(y-35)<38) {R=144;G=48;B=42;}
            put(x,y,R,G,B);
        }
        // placard silhouettes
        rect(25,28,53,37,86,80,73);
        rect(93,20,113,29,91,84,78);
    } else if(type==1){ // freedom fighters in field
        rect(8,10,w-9,70,172,165,149);
        for(int i=0;i<5;i++){
            int x=30+i*25, gy=79-(i%2)*3;
            circle(x,gy-24,5,55,50,45);
            rect(x-5,gy-19,x+5,gy-3,65,60,53);
            line(x-4,gy-10,x-13,gy-24,56,52,47,1);
            line(x+4,gy-10,x+14,gy-26,56,52,47,1);
            line(x-3,gy-3,x-8,gy+5,63,58,50,1);
            line(x+3,gy-3,x+9,gy+5,63,58,50,1);
            // long rifle silhouette
            line(x+10,gy-25,x+29,gy-47,36,33,29,1);
            line(x+27,gy-48,x+34,gy-54,35,32,28,1);
        }
        // vegetation silhouettes
        for(int x=10;x<w-8;x+=9){
            line(x,74,x-3,57+(x%11),62,78,53,1);
            line(x,74,x+4,60+(x%9),66,80,55,1);
        }
    } else if(type==2){ // river crossing / boat
        rect(8,8,w-9,58,171,174,166);
        for(int y=61;y<82;y++) for(int x=8;x<w-8;x++){
            int wave=72+int(12*std::sin(x*0.14f+y*0.12f));
            int c=(y<wave)?126:104;
            put(x,y,c-6,c+8,c+14);
        }
        // long wooden boat
        for(int x=24;x<138;x++){
            float q=float(x-24)/114.0f;
            int top=int(60-10*std::sin(q*PI));
            for(int y=top;y<top+6;y++) put(x,y,68,48,37);
        }
        for(int i=0;i<6;i++){
            int x=39+i*16;
            circle(x,52-(i%2)*3,4,54,49,44);
            rect(x-4,57-(i%2)*3,x+4,66-(i%2)*3,69,62,56);
        }
        line(84,18,84,55,51,48,41,2);
        line(84,18,101,36,51,48,41,1);
        rect(86,21,100,35,153,150,139);
    } else if(type==3){ // displaced civilians / refugee movement
        rect(8,8,w-9,61,171,173,166);
        // temporary shelters
        for(int i=0;i<4;i++){
            int x=18+i*34;
            for(int k=0;k<18;k++){
                int yy=48-k/2;
                line(x-k/2,yy,x+10-k/2,yy,121,98,69,1);
            }
            rect(x-8,50,x+18,69,132,106,75);
        }
        // civilians carrying bundles and moving together
        for(int i=0;i<7;i++){
            int x=15+i*20, gy=86-(i%2)*3;
            circle(x,gy-16,4,60,56,52);
            rect(x-4,gy-12,x+4,gy+2,77,69,61);
            line(x-2,gy,x-7,gy+9,61,56,50,1);
            line(x+2,gy,x+7,gy+9,61,56,50,1);
            if(i%2==0) circle(x+9,gy-2,5,47,48,42);
        }
        // a distant warning / military-post silhouette
        line(126,18,126,57,54,50,43,2);
        rect(127,21,150,33,73,68,59);
        line(138,33,138,55,61,57,53,1);
    } else if(type==4){ // victory gathering / flags
        rect(8,8,w-9,78,186,179,162);
        for(int i=0;i<22;i++){
            int x=12+i*6, y=72-(i%5)*4;
            circle(x,y-13,3,55,51,46);
            rect(x-2,y-10,x+2,y,69,63,56);
            if(i%4==0) line(x,y,x,38,53,49,42,1);
        }
        line(119,16,119,72,55,50,42,2);
        for(int yy=20;yy<42;yy++) for(int xx=120;xx<151;xx++){
            int R=33,G=80,B=57;
            if((xx-135)*(xx-135)+(yy-31)*(yy-31)<34){R=152;G=50;B=45;}
            put(xx,yy,R,G,B);
        }
        // skyline / crowd banners
        rect(10,83,149,93,83,75,64);
    } else if(type==5){ // archive / document / radio room
        rect(12,14,92,78,186,177,162);
        rect(22,21,80,69,217,209,188);
        for(int k=0;k<6;k++) line(28,29+k*6,74,29+k*6,86,78,67,1);
        rect(103,45,142,76,76,69,57);
        rect(108,52,136,72,54,53,48);
        for(int k=0;k<4;k++) circle(113+k*7,58,2,163,148,112);
        line(130,52,142,31,48,45,40,1);
        circle(118,35,5,61,55,49);
        line(118,40,115,53,55,50,45,1);
    } else if(type==6){ // historic rally / 7 March style podium
        rect(8,8,w-9,72,180,176,164);
        rect(20,18,110,72,90,83,69);
        rect(58,48,96,73,55,50,43);
        rect(47,35,107,50,61,56,48);
        circle(76,27,5,56,52,47);
        rect(71,32,81,49,64,58,50);
        line(72,38,56,29,58,53,46,1);
        line(80,38,94,27,58,53,46,1);
        for(int i=0;i<16;i++){
            int x=14+i*9, gy=88-(i%4)*3;
            circle(x,gy-13,3,55,51,46);
            rect(x-2,gy-10,x+2,gy,72,65,57);
        }
        line(128,20,128,76,52,48,40,2);
        for(int y=24;y<48;y++) for(int x=129;x<151;x++){
            int R=34,G=77,B=57;
            if((x-140)*(x-140)+(y-36)*(y-36)<34){R=149;G=49;B=44;}
            put(x,y,R,G,B);
        }
    } else if(type==7){ // victory / surrender / 16 December memory
        rect(8,8,w-9,79,181,178,164);
        rect(8,72,w-9,86,91,83,69);
        line(78,17,78,77,52,48,40,2);
        for(int y=21;y<46;y++) for(int x=79;x<110;x++){
            int R=34,G=78,B=57;
            if((x-94)*(x-94)+(y-34)*(y-34)<44){R=152;G=51;B=46;}
            put(x,y,R,G,B);
        }
        for(int i=0;i<18;i++){
            int x=14+i*8, gy=82-(i%5)*3;
            circle(x,gy-14,3,57,53,48);
            rect(x-2,gy-11,x+2,gy,72,65,57);
            if(i%5==0) line(x,gy-10,x,gy-27,52,48,42,1);
        }
        rect(112,53,145,72,78,70,59);
        line(116,58,140,58,53,49,42,1);
        line(116,64,137,64,53,49,42,1);
    } else { // archive fallback
        rect(12,14,92,78,186,177,162);
        rect(22,21,80,69,217,209,188);
        for(int k=0;k<6;k++) line(28,29+k*6,74,29+k*6,86,78,67,1);
        rect(103,45,142,76,76,69,57);
        rect(108,52,136,72,54,53,48);
        for(int k=0;k<4;k++) circle(113+k*7,58,2,163,148,112);
        line(130,52,142,31,48,45,40,1);
        circle(118,35,5,61,55,49);
        line(118,40,115,53,55,50,45,1);
    }
    // vignette and archival border
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        if(x<5||x>w-6||y<5||y>h-6) put(x,y,91,69,48);
        else if(((x-80)*(x-80)+(y-55)*(y-55))>6500){
            int i=(y*w+x)*3;
            d[i]=(unsigned char)(d[i]*0.84f); d[i+1]=(unsigned char)(d[i+1]*0.84f); d[i+2]=(unsigned char)(d[i+2]*0.84f);
        }
    }
    return uploadTexture(d,w,h);
}

GLuint makeBangladeshMapTexture(int w=256,int h=190) {
    std::vector<unsigned char> d(w*h*3,0);
    auto put=[&](int x,int y,int R,int G,int B){
        if(x<0||x>=w||y<0||y>=h) return;
        int i=(y*w+x)*3;
        d[i]=(unsigned char)clampf((float)R,0,255);
        d[i+1]=(unsigned char)clampf((float)G,0,255);
        d[i+2]=(unsigned char)clampf((float)B,0,255);
    };
    auto line=[&](int x0,int y0,int x1,int y1,int R,int G,int B,int thick=1){
        int dx=std::abs(x1-x0), dy=std::abs(y1-y0), steps=std::max(dx,dy);
        if(steps==0){ put(x0,y0,R,G,B); return; }
        for(int k=0;k<=steps;k++){
            float t=float(k)/steps;
            int x=int(std::round(x0+(x1-x0)*t));
            int y=int(std::round(y0+(y1-y0)*t));
            for(int oy=-thick;oy<=thick;oy++) for(int ox=-thick;ox<=thick;ox++)
                put(x+ox,y+oy,R,G,B);
        }
    };
    auto circle=[&](int cx,int cy,int r,int R,int G,int B){
        for(int y=cy-r;y<=cy+r;y++) for(int x=cx-r;x<=cx+r;x++)
            if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r) put(x,y,R,G,B);
    };
    auto rect=[&](int x0,int y0,int x1,int y1,int R,int G,int B){
        for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) put(x,y,R,G,B);
    };

    // Warm museum-map paper with very light grain.
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        int grain=((x*7+y*11)%9)-4;
        put(x,y,242+grain,234+grain,210+grain);
    }

    // Cleaner, higher-resolution schematic outline of Bangladesh.
    // It is an educational silhouette rather than a survey-accurate border.
    const int mapOx=38;
    const int pts[][2]={
        {72,18},{88,15},{103,17},{118,21},{130,27},{138,36},{137,47},
        {145,58},{143,70},{148,80},{145,91},{151,101},{149,113},{141,124},
        {128,132},{118,139},{107,143},{96,141},{86,137},{77,132},{68,126},
        {58,122},{48,116},{40,109},{31,104},{25,96},{28,87},{22,80},
        {28,72},{24,64},{29,56},{27,47},{34,40},{44,39},{48,31},
        {58,28},{66,22}
    };
    const int npts=int(sizeof(pts)/sizeof(pts[0]));
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        bool inside=false;
        for(int i=0,j=npts-1;i<npts;j=i++){
            int xi=pts[i][0]+mapOx, yi=pts[i][1], xj=pts[j][0]+mapOx, yj=pts[j][1];
            bool inter=((yi>y)!=(yj>y)) &&
                (x < (xj-xi)*(y-yi)/float((yj==yi)?1:(yj-yi))+xi);
            if(inter) inside=!inside;
        }
        if(inside) put(x,y,57,123,72);
    }
    for(int i=0;i<npts;i++){
        int j=(i+1)%npts;
        line(pts[i][0]+mapOx,pts[i][1],pts[j][0]+mapOx,pts[j][1],31,72,45,2);
    }

    // Major river guides to make the geography read more clearly.
    const int river1[][2]={{33,58},{48,61},{64,66},{83,71},{101,77},{119,88},{138,100}};
    const int river2[][2]={{64,24},{67,39},{73,55},{79,72},{83,92},{86,112},{89,136}};
    const int river3[][2]={{112,28},{106,43},{102,59},{104,76},{111,94},{121,110},{133,123}};
    auto drawPolyline=[&](const int a[][2],int n,int R,int G,int B,int thick){
        for(int i=0;i<n-1;i++) line(a[i][0]+mapOx,a[i][1],a[i+1][0]+mapOx,a[i+1][1],R,G,B,thick);
    };
    drawPolyline(river1,7,58,126,156,1);
    drawPolyline(river2,7,58,126,156,1);
    drawPolyline(river3,7,58,126,156,1);

    // National-color focal circle near central Bangladesh.
    circle(87+mapOx,77,13,177,49,45);
    circle(87+mapOx,77,9,192,57,50);

    // Eleven lightweight sector markers; placement is schematic for readability.
    const int sx[11]={38,52,68,87,101,121,134,108,82,58,43};
    const int sy[11]={63,49,42,51,68,75,94,103,112,98,84};
    for(int k=0;k<11;k++){
        circle(sx[k]+mapOx,sy[k],3,226,188,66);
        circle(sx[k]+mapOx,sy[k],1,75,60,38);
    }

    // Legend strip + compass.
    rect(12,156,243,176,61,49,39);
    rect(18,162,30,169,57,123,72);
    rect(34,162,46,169,177,49,45);
    line(224,34,224,57,55,48,37,1);
    line(224,34,219,43,55,48,37,1);
    line(224,34,229,43,55,48,37,1);
    return uploadTexture(d,w,h);
}

GLuint makeBangladeshWarMapDetailTexture(int w=220,int h=165) {
    std::vector<unsigned char> d(w*h*3,0);
    auto put=[&](int x,int y,int R,int G,int B){
        if(x<0||x>=w||y<0||y>=h) return;
        int i=(y*w+x)*3;
        d[i]=(unsigned char)clampf((float)R,0,255);
        d[i+1]=(unsigned char)clampf((float)G,0,255);
        d[i+2]=(unsigned char)clampf((float)B,0,255);
    };
    auto line=[&](int x0,int y0,int x1,int y1,int R,int G,int B,int thick=1){
        int dx=std::abs(x1-x0), dy=std::abs(y1-y0), steps=std::max(dx,dy);
        if(steps==0){ put(x0,y0,R,G,B); return; }
        for(int k=0;k<=steps;k++){
            float t=float(k)/steps;
            int x=int(std::round(x0+(x1-x0)*t));
            int y=int(std::round(y0+(y1-y0)*t));
            for(int oy=-thick;oy<=thick;oy++) for(int ox=-thick;ox<=thick;ox++) put(x+ox,y+oy,R,G,B);
        }
    };
    auto circle=[&](int cx,int cy,int r,int R,int G,int B){
        for(int y=cy-r;y<=cy+r;y++) for(int x=cx-r;x<=cx+r;x++)
            if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r) put(x,y,R,G,B);
    };
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) put(x,y,235,228,202);

    // Compact map silhouette, visually simplified for a wall panel.
    const int mapOx=18;
    const int pts[][2]={
        {66,17},{84,15},{105,18},{121,23},{130,33},{127,44},{136,55},{133,68},
        {139,80},{136,94},{144,105},{140,117},{128,127},{114,135},{99,138},
        {84,134},{69,129},{56,121},{43,113},{31,101},{27,90},{32,79},{26,68},
        {31,57},{28,47},{37,39},{45,37},{51,28},{59,24}
    };
    const int npts=int(sizeof(pts)/sizeof(pts[0]));
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        bool inside=false;
        for(int i=0,j=npts-1;i<npts;j=i++){
            int xi=pts[i][0]+mapOx,yi=pts[i][1],xj=pts[j][0]+mapOx,yj=pts[j][1];
            bool inter=((yi>y)!=(yj>y)) && (x < (xj-xi)*(y-yi)/float((yj==yi)?1:(yj-yi))+xi);
            if(inter) inside=!inside;
        }
        if(inside) put(x,y,49,112,67);
    }
    for(int i=0;i<npts;i++){
        int j=(i+1)%npts;
        line(pts[i][0]+mapOx,pts[i][1],pts[j][0]+mapOx,pts[j][1],28,66,41,2);
    }

    // Main communication corridors / rivers and a highlighted Dhaka node.
    const int route1[][2]={{34,60},{53,61},{74,68},{96,78},{118,91},{135,108}};
    const int route2[][2]={{71,21},{72,40},{80,61},{88,80},{93,101},{98,128}};
    for(int i=0;i<5;i++) line(route1[i][0]+mapOx,route1[i][1],route1[i+1][0]+mapOx,route1[i+1][1],59,121,155,1);
    for(int i=0;i<5;i++) line(route2[i][0]+mapOx,route2[i][1],route2[i+1][0]+mapOx,route2[i+1][1],59,121,155,1);
    circle(88+mapOx,80,6,181,51,46);
    circle(88+mapOx,80,2,244,225,90);

    // Sector network: 11 numbered-looking nodes connected by a thin route line.
    const int sx[11]={39,52,66,82,101,121,134,111,91,67,48};
    const int sy[11]={64,50,43,50,63,73,93,105,116,101,86};
    for(int i=0;i<10;i++) line(sx[i]+mapOx,sy[i],sx[i+1]+mapOx,sy[i+1],184,145,58,1);
    for(int k=0;k<11;k++){
        circle(sx[k]+mapOx,sy[k],3,225,185,65);
        circle(sx[k]+mapOx,sy[k],1,73,57,37);
    }

    // Clean legend blocks.
    for(int x=10;x<210;x++) { put(x,146,66,53,42); put(x,147,66,53,42); }
    return uploadTexture(d,w,h);
}

void createTextures() {
    texFloor = makeMarbleTexture();
    texWall = makeWallTexture();
    texWood = makeWoodTexture();
    texCeiling = makeCeilingTexture();
    texCarpet = makeCarpetTexture();
    // Reuse the existing procedural Bangladesh village texture as a distant
    // exterior landscape visible through the real upper-floor window openings.
    texVillage = makeVillageTexture();
    texWarMap = makeBangladeshMapTexture();
    texWarMapDetail = makeBangladeshWarMapDetailTexture();
    texWarPhoto1 = makeLiberationPhotoTexture(0);
    texWarPhoto2 = makeLiberationPhotoTexture(1);
    texWarPhoto3 = makeLiberationPhotoTexture(2);
    texWarPhoto4 = makeLiberationPhotoTexture(3);
    texWarPhoto5 = makeLiberationPhotoTexture(4);
    texWarPhoto6 = makeLiberationPhotoTexture(5);
    texWarPhoto7 = makeLiberationPhotoTexture(6);
    texWarPhoto8 = makeLiberationPhotoTexture(7);
}

// ================================================================
// 3D text, labels and paintings
// ================================================================

void drawStrokeText(const std::string& text, float x,float y,float z,float scale,
                    float r,float g,float b) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(scale,scale,scale);
    setMaterial(r,g,b,12,0.15f);
    for(char c: text) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
}

void drawPainting(float x,float y,float z,float w,float h,GLuint texture,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // Wooden frame. Step 6: keep it shallow so artwork reads as
    // wall-mounted instead of floating several centimeters in front of the wall.
    drawTexturedBox(0, h*0.5f+0.10f, 0, w+0.35f,0.20f,0.14f,texWood);
    drawTexturedBox(0,-h*0.5f-0.10f, 0, w+0.35f,0.20f,0.14f,texWood);
    drawTexturedBox(-w*0.5f-0.10f,0,0,0.20f,h,0.14f,texWood);
    drawTexturedBox( w*0.5f+0.10f,0,0,0.20f,h,0.14f,texWood);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,texture);
    setMaterial(1,1,1,8,0.08f);
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glTexCoord2f(0,0); glVertex3f(-w*0.5f,-h*0.5f,0.078f);
    glTexCoord2f(1,0); glVertex3f( w*0.5f,-h*0.5f,0.078f);
    glTexCoord2f(1,1); glVertex3f( w*0.5f, h*0.5f,0.078f);
    glTexCoord2f(0,1); glVertex3f(-w*0.5f, h*0.5f,0.078f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}


// Curated wall artwork: framed image + museum caption + picture light.
// The caption is intentionally small so visitors need to walk close to read it,
// which makes the first-person museum experience feel more natural.
void drawWallArtwork(float x,float y,float z,float w,float h,GLuint texture,
                     float rotY,const std::string& title,const std::string& collection,
                     bool addPictureLight=true) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    drawPainting(0,0,0,w,h,texture,0);

    // Small museum label below the frame.
    float plaqueW=clampf(1.85f + 0.075f*float(title.size()),2.4f,4.3f);
    drawBox(0,-h*0.5f-0.58f,0.10f,plaqueW,0.48f,0.07f,0.92f,0.90f,0.84f,12,0.06f);
    drawBox(0,-h*0.5f-0.58f,0.145f,plaqueW-0.08f,0.40f,0.020f,0.985f,0.975f,0.93f,10,0.03f);

    glPushMatrix();
    glTranslatef(-plaqueW*0.43f,-h*0.5f-0.52f,0.165f);
    glScalef(0.00122f,0.00122f,0.00122f);
    setMaterial(0.025f,0.023f,0.020f,8,0.02f);
    for(char c:title) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-plaqueW*0.43f,-h*0.5f-0.73f,0.165f);
    glScalef(0.00078f,0.00078f,0.00078f);
    setMaterial(0.10f,0.095f,0.085f,8,0.02f);
    for(char c:collection) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    // Two small concealed mounting tabs visually connect the frame to the wall.
    drawBox(-w*0.28f,h*0.5f+0.18f,-0.055f,0.16f,0.30f,0.035f,0.22f,0.18f,0.13f,24,0.18f);
    drawBox( w*0.28f,h*0.5f+0.18f,-0.055f,0.16f,0.30f,0.035f,0.22f,0.18f,0.13f,24,0.18f);

    if(addPictureLight){
        float barW=std::min(w*0.62f,3.1f);
        // Brass picture-light arm and warm lamp body.
        drawBox(0,h*0.5f+0.62f,0.16f,barW,0.10f,0.11f,0.45f,0.33f,0.10f,48,0.55f);
        drawBox(-barW*0.38f,h*0.5f+0.40f,0.15f,0.08f,0.40f,0.08f,0.34f,0.24f,0.09f,35,0.35f);
        drawBox( barW*0.38f,h*0.5f+0.40f,0.15f,0.08f,0.40f,0.08f,0.34f,0.24f,0.09f,35,0.35f);
        drawScaledSphere(0,h*0.5f+0.52f,0.24f,barW*0.42f,0.11f,0.07f,1.0f,0.79f,0.38f,55,0.65f);
    }

    glPopMatrix();
}

// Large curator panel used inside a themed room. It gives each gallery a clear
// identity instead of making the museum feel like random pictures on walls.
void drawCollectionIntroPanel(const std::string& title,const std::string& subtitle,
                              float x,float y,float z,float rotY,float width=6.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawBox(0,0,0,width,1.55f,0.11f,0.92f,0.90f,0.84f,18,0.10f);
    drawBox(-width*0.47f,0,0.075f,0.06f,1.30f,0.03f,0.58f,0.38f,0.11f,35,0.35f);

    glPushMatrix();
    glTranslatef(-width*0.42f,0.20f,0.10f);
    glScalef(0.00225f,0.00225f,0.00225f);
    setMaterial(0.19f,0.16f,0.12f,18,0.10f);
    for(char c:title) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-width*0.42f,-0.34f,0.10f);
    glScalef(0.00103f,0.00103f,0.00103f);
    setMaterial(0.39f,0.36f,0.31f,12,0.08f);
    for(char c:subtitle) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glPopMatrix();
}

// Three-piece salon arrangement for a feature wall. The pieces are aligned to
// the same eye line, with consistent spacing and their own captions/lights.
void drawArtworkTriptych(float x,float y,float z,float rotY,
                         GLuint a,GLuint b,GLuint c,
                         const std::string& t1,const std::string& t2,const std::string& t3,
                         const std::string& collection) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawWallArtwork(-3.05f,0,0,2.35f,2.65f,a,0,t1,collection,true);
    drawWallArtwork( 0.00f,0,0,2.35f,2.65f,b,0,t2,collection,true);
    drawWallArtwork( 3.05f,0,0,2.35f,2.65f,c,0,t3,collection,true);
    glPopMatrix();
}

void drawArtworkLabel(const std::string& text, float x,float y,float z,float rotY=0) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // Step 16: compact museum label.  It is deliberately drawn UNLIT and
    // double-sided so it remains ivory/white from every viewing angle instead
    // of becoming a black rectangle when the camera moves around it.
    const float width=3.35f;
    const float h=0.66f;
    const float t=0.055f;
    const float zf=t*0.5f;

    // visible support post + foot
    drawBox(0,-0.45f,0,0.10f,0.78f,0.10f,0.50f,0.46f,0.38f,22,0.12f);
    drawBox(0,-0.82f,0,0.82f,0.09f,0.38f,0.78f,0.75f,0.68f,18,0.08f);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // front + back ivory faces
    glColor3f(0.97f,0.955f,0.90f);
    glBegin(GL_QUADS);
    glVertex3f(-width*0.5f,-h*0.5f, zf); glVertex3f( width*0.5f,-h*0.5f, zf);
    glVertex3f( width*0.5f, h*0.5f, zf); glVertex3f(-width*0.5f, h*0.5f, zf);
    glVertex3f( width*0.5f,-h*0.5f,-zf); glVertex3f(-width*0.5f,-h*0.5f,-zf);
    glVertex3f(-width*0.5f, h*0.5f,-zf); glVertex3f( width*0.5f, h*0.5f,-zf);
    glEnd();

    // thin warm-grey edges
    glColor3f(0.67f,0.63f,0.55f);
    glBegin(GL_QUADS);
    glVertex3f(-width*0.5f,-h*0.5f,-zf); glVertex3f(-width*0.5f,-h*0.5f, zf);
    glVertex3f(-width*0.5f, h*0.5f, zf); glVertex3f(-width*0.5f, h*0.5f,-zf);
    glVertex3f( width*0.5f,-h*0.5f, zf); glVertex3f( width*0.5f,-h*0.5f,-zf);
    glVertex3f( width*0.5f, h*0.5f,-zf); glVertex3f( width*0.5f, h*0.5f, zf);
    glVertex3f(-width*0.5f, h*0.5f, zf); glVertex3f( width*0.5f, h*0.5f, zf);
    glVertex3f( width*0.5f, h*0.5f,-zf); glVertex3f(-width*0.5f, h*0.5f,-zf);
    glVertex3f(-width*0.5f,-h*0.5f,-zf); glVertex3f( width*0.5f,-h*0.5f,-zf);
    glVertex3f( width*0.5f,-h*0.5f, zf); glVertex3f(-width*0.5f,-h*0.5f, zf);
    glEnd();

    // black ink on the front
    glColor3f(0.045f,0.040f,0.032f);
    glLineWidth(2.0f);
    glPushMatrix();
    glTranslatef(-width*0.43f,-0.12f,zf+0.002f);
    glScalef(0.00172f,0.00172f,0.00172f);
    for(char c:text) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    // same readable text on the back
    glPushMatrix();
    glTranslatef(0,0,-zf-0.002f);
    glRotatef(180,0,1,0);
    glTranslatef(-width*0.43f,-0.12f,0);
    glScalef(0.00172f,0.00172f,0.00172f);
    for(char c:text) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glLineWidth(1.0f);
    glColor3f(1,1,1); // never leak dark drawing colour into later objects
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Two-line museum-style information plaque used beside major exhibits.
// Step 15 cleanup: draw the sign as a simple DOUBLE-SIDED ivory panel with
// black text so no side/back ever turns into a dark or fully black block.
void drawExhibitPlaque(const std::string& title, const std::string& subtitle,
                       float x,float y,float z,float rotY=0,float width=4.35f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // slim stand / support
    drawBox(0,-0.34f,0,0.14f,0.68f,0.14f,0.72f,0.70f,0.66f,18,0.08f);
    drawBox(0,-0.68f,0,0.84f,0.07f,0.42f,0.90f,0.89f,0.84f,10,0.03f);

    // unlit panel so it stays readable from every direction
    const float h = 1.04f;
    const float t = 0.06f;
    const float zf = t * 0.5f;
    glDisable(GL_LIGHTING);

    // front and back main faces
    glColor3f(0.95f,0.93f,0.87f);
    glBegin(GL_QUADS);
    // front
    glVertex3f(-width*0.5f, -h*0.5f,  zf);
    glVertex3f( width*0.5f, -h*0.5f,  zf);
    glVertex3f( width*0.5f,  h*0.5f,  zf);
    glVertex3f(-width*0.5f,  h*0.5f,  zf);
    // back
    glVertex3f( width*0.5f, -h*0.5f, -zf);
    glVertex3f(-width*0.5f, -h*0.5f, -zf);
    glVertex3f(-width*0.5f,  h*0.5f, -zf);
    glVertex3f( width*0.5f,  h*0.5f, -zf);
    glEnd();

    // edges
    glColor3f(0.83f,0.80f,0.73f);
    glBegin(GL_QUADS);
    // left
    glVertex3f(-width*0.5f,-h*0.5f,-zf); glVertex3f(-width*0.5f,-h*0.5f, zf);
    glVertex3f(-width*0.5f, h*0.5f, zf); glVertex3f(-width*0.5f, h*0.5f,-zf);
    // right
    glVertex3f( width*0.5f,-h*0.5f, zf); glVertex3f( width*0.5f,-h*0.5f,-zf);
    glVertex3f( width*0.5f, h*0.5f,-zf); glVertex3f( width*0.5f, h*0.5f, zf);
    // top
    glVertex3f(-width*0.5f, h*0.5f, zf); glVertex3f( width*0.5f, h*0.5f, zf);
    glVertex3f( width*0.5f, h*0.5f,-zf); glVertex3f(-width*0.5f, h*0.5f,-zf);
    // bottom
    glVertex3f(-width*0.5f,-h*0.5f,-zf); glVertex3f( width*0.5f,-h*0.5f,-zf);
    glVertex3f( width*0.5f,-h*0.5f, zf); glVertex3f(-width*0.5f,-h*0.5f, zf);
    glEnd();

    // decorative border lines
    glLineWidth(1.8f);
    glColor3f(0.40f,0.31f,0.14f);
    for(float zz : {zf+0.001f, -zf-0.001f}) {
        glBegin(GL_LINE_LOOP);
        glVertex3f(-width*0.5f+0.08f, -h*0.5f+0.08f, zz);
        glVertex3f( width*0.5f-0.08f, -h*0.5f+0.08f, zz);
        glVertex3f( width*0.5f-0.08f,  h*0.5f-0.08f, zz);
        glVertex3f(-width*0.5f+0.08f,  h*0.5f-0.08f, zz);
        glEnd();
    }

    // front text
    glColor3f(0.05f,0.04f,0.03f);
    glPushMatrix();
    glTranslatef(-width*0.42f,0.08f,zf+0.003f);
    glScalef(0.00162f,0.00162f,0.00162f);
    for(char c:title) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glColor3f(0.20f,0.17f,0.14f);
    glPushMatrix();
    glTranslatef(-width*0.42f,-0.27f,zf+0.003f);
    glScalef(0.00105f,0.00105f,0.00105f);
    for(char c:subtitle) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    // back text (mirrored by 180-degree rotation)
    glColor3f(0.05f,0.04f,0.03f);
    glPushMatrix();
    glTranslatef(0,0,-zf-0.003f);
    glRotatef(180,0,1,0);
    glTranslatef(-width*0.42f,0.08f,0);
    glScalef(0.00162f,0.00162f,0.00162f);
    for(char c:title) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glColor3f(0.20f,0.17f,0.14f);
    glPushMatrix();
    glTranslatef(0,0,-zf-0.003f);
    glRotatef(180,0,1,0);
    glTranslatef(-width*0.42f,-0.27f,0);
    glScalef(0.00105f,0.00105f,0.00105f);
    for(char c:subtitle) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glLineWidth(1.0f);
    glColor3f(1,1,1);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawPlinthFrontLabel(const std::string& title,
                          float x,float y,float z,float rotY,
                          float width=3.8f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // Step 14 fix: keep the ORIGINAL WHITE PLINTH completely visible.
    // No extra dark plate/card is drawn here.  The exhibit name is written
    // directly on the front face like black pen/ink on a museum plinth.
    glPushMatrix();
    glTranslatef(-width*0.45f,-0.145f,0.018f);
    glScalef(0.00205f,0.00205f,0.00205f);
    glDisable(GL_LIGHTING);
    glLineWidth(2.4f);
    glColor3f(0.015f,0.012f,0.010f);
    for(char c:title) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glLineWidth(1.0f);
    glColor3f(1,1,1);
    glEnable(GL_LIGHTING);
    glPopMatrix();

    glPopMatrix();
}



// ================================================================
// Decorative museum helpers
// ================================================================

void drawRoomSign(const std::string& text,float x,float y,float z,float rotY,float width=4.8f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawBox(0,0,0,width,0.72f,0.18f,0.10f,0.075f,0.045f,55,0.55f);
    drawBox(0,0,0.11f,width-0.18f,0.54f,0.05f,0.28f,0.22f,0.12f,55,0.55f);
    glTranslatef(-width*0.43f,-0.12f,0.16f);
    glScalef(0.00205f,0.00205f,0.00205f);
    setMaterial(0.96f,0.78f,0.30f,50,0.6f);
    for(char c:text) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
}

void drawDoorFrameSide(float x,float doorZ) {
    // Step 11: the old 0.72-wide jambs looked like unsupported floating slabs
    // when viewed from the lobby/corridor at an angle. Use thin, flush,
    // floor-anchored architrave strips instead.
    const float side=(x<0)?1.0f:-1.0f;
    const float frameX=x+0.015f*side;
    const float jambZ=2.50f;

    drawTexturedBox(frameX,3.45f,doorZ-jambZ,0.18f,6.90f,0.24f,texWood,28);
    drawTexturedBox(frameX,3.45f,doorZ+jambZ,0.18f,6.90f,0.24f,texWood,28);

    drawBox(frameX,0.18f,doorZ-jambZ,0.25f,0.36f,0.34f,
            0.58f,0.42f,0.14f,55,0.60f);
    drawBox(frameX,0.18f,doorZ+jambZ,0.25f,0.36f,0.34f,
            0.58f,0.42f,0.14f,55,0.60f);

    drawTexturedBox(frameX,6.98f,doorZ,0.18f,0.26f,5.20f,texWood,28);
    drawBox(frameX,7.18f,doorZ,0.22f,0.11f,5.30f,
            0.64f,0.48f,0.15f,55,0.65f);
}

void drawWallLamp(float x,float y,float z,float rotY=0) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawTexturedBox(0,0,0,0.50f,0.65f,0.18f,texWood,25);
    drawCylinderY(0,-0.15f,0.20f,0.06f,0.55f,0.42f,0.34f,0.18f,40,0.5f);
    drawScaledSphere(0,0.30f,0.31f,0.28f,0.34f,0.20f,1.0f,0.78f,0.34f,65,0.8f);
    glPopMatrix();
}

void drawChandelier(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,8.9f,z);
    glScalef(scale,scale,scale);
    // Slow kinetic rotation: this makes centralRotation visibly meaningful
    // while the faster ceiling fans remain the primary animated objects.
    glRotatef(centralRotation,0,1,0);
    drawCylinderY(0,0,0,0.08f,0.80f,0.34f,0.26f,0.10f,45,0.6f);
    glTranslatef(0,-0.10f,0);
    setMaterial(0.55f,0.40f,0.12f,70,0.8f);
    glRotatef(90,1,0,0);
    glutSolidTorus(0.08f,0.85f,18,36);
    glRotatef(-90,1,0,0);
    for(int i=0;i<8;i++){
        float a=i*2.0f*PI/8.0f;
        float bx=0.85f*std::cos(a), bz=0.85f*std::sin(a);
        drawSphere(bx,-0.22f,bz,0.16f,1.0f,0.78f,0.32f,75,0.9f);
    }
    glPopMatrix();
}

// A clearly visible continuously rotating exhibit for the assignment requirement.
// Unlike a ceiling fan, this stands at eye level so the animation is obvious during a demo.
void drawRotatingVictoryEmblem(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glScalef(scale,scale,scale);

    // Museum plinth and central axle.
    drawTexturedBox(0,0.42f,0,2.45f,0.84f,2.45f,texWood,34);
    drawBox(0,0.88f,0,2.58f,0.10f,2.58f,0.58f,0.43f,0.14f,58,0.62f);
    drawCylinderY(0,0.88f,0,0.11f,2.15f,0.52f,0.40f,0.15f,40,0.55f);

    // The entire emblem rotates continuously around the vertical axis.
    glPushMatrix();
    glTranslatef(0,2.28f,0);
    glRotatef(centralRotation*1.35f,0,1,0);

    // Green outer ring and gold cross-bars make the rotation readable from a distance.
    setMaterial(0.05f,0.36f,0.20f,65,0.62f);
    glutSolidTorus(0.12f,0.92f,18,42);
    drawBox(0,0,0,1.80f,0.12f,0.16f,0.72f,0.53f,0.12f,60,0.70f);
    drawBox(0,0,0,0.16f,1.80f,0.12f,0.72f,0.53f,0.12f,60,0.70f);

    // Red central disc evokes the Bangladesh flag without using a flat billboard.
    drawScaledSphere(0,0,0.08f,0.42f,0.42f,0.14f,0.76f,0.05f,0.06f,58,0.48f);

    // Asymmetric small markers make motion especially clear.
    drawSphere(0.86f,0.26f,0.12f,0.12f,0.95f,0.78f,0.28f,55,0.70f);
    drawSphere(-0.58f,-0.66f,0.12f,0.10f,0.95f,0.78f,0.28f,55,0.70f);
    glPopMatrix();

    drawArtworkLabel("ROTATING VICTORY EMBLEM",0.0f,0.72f,1.68f,180);
    glPopMatrix();
}


// ================================================================
// Dense historical exhibit additions
// ================================================================
// These displays are intentionally museum-style, non-functional geometry.
// They fill large empty zones without blocking the main circulation route.

void drawRopeBarrierRect(float sx,float sz) {
    const float px=sx*0.5f, pz=sz*0.5f;
    const Vec3 p[4]={Vec3(-px,0.0f,-pz),Vec3(px,0.0f,-pz),Vec3(px,0.0f,pz),Vec3(-px,0.0f,pz)};
    for(int i=0;i<4;i++) {
        drawCylinderY(p[i].x,0.08f,p[i].z,0.045f,0.92f,0.34f,0.25f,0.10f,20,0.35f);
        drawSphere(p[i].x,1.02f,p[i].z,0.075f,0.72f,0.53f,0.16f,42,0.60f);
    }
    for(int i=0;i<4;i++) {
        Vec3 a=p[i]; Vec3 b=p[(i+1)%4];
        a.y=b.y=0.82f;
        drawCylinderBetween(a,b,0.026f,0.48f,0.08f,0.06f);
    }
}

void drawArtilleryCannonDisplay(float x,float z,float scale=1.0f,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);
    glScalef(scale,scale,scale);

    // Low exhibit platform.
    drawTexturedBox(0,0.13f,0,6.9f,0.26f,4.9f,texFloor,32);
    drawBox(0,0.29f,0,6.95f,0.08f,4.95f,0.47f,0.39f,0.24f,40,0.32f);

    // Carriage and axle.
    drawBox(0,0.82f,0.05f,2.55f,0.42f,2.25f,0.20f,0.25f,0.14f,30,0.24f);
    drawCylinderBetween(Vec3(-1.60f,0.78f,0.10f),Vec3(1.60f,0.78f,0.10f),0.12f,0.12f,0.13f,0.10f);

    // Two large wheels.
    for(float wx : {-1.48f,1.48f}) {
        glPushMatrix();
        glTranslatef(wx,0.86f,0.10f);
        glRotatef(90.0f,0,1,0);
        setMaterial(0.10f,0.11f,0.09f,28,0.22f);
        glutSolidTorus(0.13f,0.72f,16,34);
        glPopMatrix();
        drawSphere(wx,0.86f,0.10f,0.16f,0.28f,0.29f,0.22f,30,0.32f);
    }

    // Breech and elevated barrel - display geometry only.
    drawBox(0,1.38f,0.10f,0.82f,0.66f,1.05f,0.22f,0.28f,0.16f,34,0.28f);
    drawCylinderBetween(Vec3(0,1.55f,0.40f),Vec3(0,2.15f,3.48f),0.17f,0.18f,0.23f,0.13f);
    drawCylinderBetween(Vec3(0,2.15f,3.48f),Vec3(0,2.23f,4.05f),0.22f,0.14f,0.17f,0.10f);

    // Split trail and stabilizers.
    drawCylinderBetween(Vec3(-0.34f,0.65f,-0.85f),Vec3(-1.05f,0.34f,-2.05f),0.09f,0.22f,0.26f,0.15f);
    drawCylinderBetween(Vec3( 0.34f,0.65f,-0.85f),Vec3( 1.05f,0.34f,-2.05f),0.09f,0.22f,0.26f,0.15f);
    drawBox(-1.05f,0.22f,-2.05f,0.66f,0.12f,0.40f,0.16f,0.18f,0.12f,24,0.18f);
    drawBox( 1.05f,0.22f,-2.05f,0.66f,0.12f,0.40f,0.16f,0.18f,0.12f,24,0.18f);

    drawRopeBarrierRect(6.1f,4.2f);
    glPopMatrix();
}

void draw1971MilitaryJeepDisplay(float x,float z,float scale=1.0f,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);
    glScalef(scale,scale,scale);

    drawTexturedBox(0,0.13f,0,7.0f,0.26f,4.8f,texFloor,32);
    drawBox(0,0.29f,0,7.05f,0.08f,4.85f,0.47f,0.39f,0.24f,40,0.32f);

    // Olive-drab museum model body.
    const float R=0.19f,G=0.24f,B=0.13f;
    drawBox(0,1.00f,0,4.45f,0.70f,2.35f,R,G,B,28,0.22f);
    drawBox(0,1.43f,0.62f,4.25f,0.46f,1.10f,R*0.92f,G*0.92f,B*0.92f,28,0.20f);
    drawBox(0,1.43f,-0.92f,3.85f,0.34f,0.48f,R*0.85f,G*0.85f,B*0.85f,28,0.18f);

    // Open cabin, seats and simple dashboard.
    drawBox(-0.86f,1.73f,-0.18f,0.82f,0.76f,0.72f,0.18f,0.19f,0.14f,22,0.16f);
    drawBox( 0.86f,1.73f,-0.18f,0.82f,0.76f,0.72f,0.18f,0.19f,0.14f,22,0.16f);
    drawBox(0,1.86f,0.32f,3.20f,0.18f,0.18f,0.12f,0.15f,0.09f,28,0.20f);

    // Windshield frame and glass-like inner panel.
    drawBox(-1.48f,2.16f,0.39f,0.10f,1.20f,0.10f,0.20f,0.21f,0.15f,35,0.30f);
    drawBox( 1.48f,2.16f,0.39f,0.10f,1.20f,0.10f,0.20f,0.21f,0.15f,35,0.30f);
    drawBox(0,2.72f,0.39f,3.02f,0.10f,0.10f,0.20f,0.21f,0.15f,35,0.30f);
    setMaterialAlpha(0.33f,0.50f,0.54f,0.28f,50,0.45f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawBox(0,2.17f,0.40f,2.82f,0.92f,0.035f,0.34f,0.48f,0.52f,55,0.44f);
    glDisable(GL_BLEND);

    // Four wheels.
    for(float wx : {-1.86f,1.86f}) for(float wz : {-0.77f,0.77f}) {
        glPushMatrix();
        glTranslatef(wx,0.70f,wz);
        glRotatef(90.0f,0,1,0);
        setMaterial(0.055f,0.055f,0.050f,20,0.12f);
        glutSolidTorus(0.16f,0.48f,14,28);
        glPopMatrix();
        drawSphere(wx,0.70f,wz,0.13f,0.30f,0.31f,0.24f,26,0.20f);
    }

    // Bumpers, lamps and spare wheel.
    drawBox(0,0.92f,1.27f,4.55f,0.13f,0.16f,0.12f,0.13f,0.10f,25,0.18f);
    drawSphere(-1.28f,1.31f,1.24f,0.18f,0.92f,0.78f,0.40f,52,0.62f);
    drawSphere( 1.28f,1.31f,1.24f,0.18f,0.92f,0.78f,0.40f,52,0.62f);
    glPushMatrix();
    glTranslatef(0,1.48f,-1.22f); glRotatef(90,0,1,0);
    setMaterial(0.055f,0.055f,0.050f,20,0.12f); glutSolidTorus(0.13f,0.42f,14,28);
    glPopMatrix();

    drawRopeBarrierRect(6.2f,4.1f);
    glPopMatrix();
}

void drawMortarAndCrateDisplay(float x,float z,float scale=1.0f,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z); glRotatef(rotY,0,1,0); glScalef(scale,scale,scale);
    drawTexturedBox(0,0.10f,0,4.5f,0.20f,3.6f,texFloor,26);

    // Tripod and upward tube - static museum model.
    drawCylinderBetween(Vec3(0,0.30f,0),Vec3(0,2.20f,0.45f),0.11f,0.17f,0.22f,0.13f);
    drawCylinderBetween(Vec3(0,0.85f,0.18f),Vec3(-0.95f,0.18f,-0.60f),0.055f,0.18f,0.21f,0.13f);
    drawCylinderBetween(Vec3(0,0.85f,0.18f),Vec3( 0.95f,0.18f,-0.60f),0.055f,0.18f,0.21f,0.13f);
    drawCylinderBetween(Vec3(0,0.85f,0.18f),Vec3( 0.00f,0.18f, 0.95f),0.055f,0.18f,0.21f,0.13f);
    drawScaledSphere(0,0.22f,0.86f,0.54f,0.09f,0.40f,0.14f,0.16f,0.11f,25,0.18f);

    // Ammunition/supply crates as contextual museum props.
    drawTexturedBox(-1.35f,0.42f,0.58f,1.10f,0.72f,0.72f,texWood,22);
    drawTexturedBox( 1.36f,0.42f,0.58f,1.10f,0.72f,0.72f,texWood,22);
    drawBox(-1.35f,0.78f,0.58f,1.14f,0.06f,0.76f,0.42f,0.31f,0.12f,24,0.20f);
    drawBox( 1.36f,0.78f,0.58f,1.14f,0.06f,0.76f,0.42f,0.31f,0.12f,24,0.20f);
    glPopMatrix();
}

void drawFieldCommandPostDiorama(float x,float z,float scale=1.0f,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z); glRotatef(rotY,0,1,0); glScalef(scale,scale,scale);
    drawTexturedBox(0,0.10f,0,5.8f,0.20f,4.5f,texFloor,28);

    // Table, field radio box and map surface.
    drawTexturedBox(0,1.05f,0.15f,3.15f,0.18f,1.70f,texWood,28);
    for(float sx : {-1.25f,1.25f}) for(float sz : {-0.48f,0.70f})
        drawBox(sx,0.55f,sz,0.14f,1.10f,0.14f,0.22f,0.16f,0.09f,22,0.14f);
    drawBox(-0.72f,1.42f,0.10f,0.92f,0.68f,0.70f,0.18f,0.23f,0.16f,28,0.22f);
    for(int i=0;i<3;i++) drawSphere(-0.98f+i*0.25f,1.44f,0.47f,0.06f,0.70f,0.55f,0.20f,30,0.36f);
    drawCylinderBetween(Vec3(-0.38f,1.70f,0.12f),Vec3(-0.10f,2.78f,0.12f),0.028f,0.15f,0.17f,0.12f);
    drawTexturedBox(0.72f,1.20f,0.10f,1.10f,0.05f,0.82f,texWarMapDetail,8);

    // Sandbag arc at the back of the display.
    for(int i=-3;i<=3;i++) {
        float px=i*0.58f;
        drawScaledSphere(px,0.40f,-1.38f,0.34f,0.20f,0.23f,0.52f,0.43f,0.28f,12,0.08f);
        if(i>-3 && i<3)
            drawScaledSphere(px+0.28f,0.72f,-1.38f,0.32f,0.18f,0.22f,0.50f,0.41f,0.27f,12,0.08f);
    }
    glPopMatrix();
}

void drawWallWeaponRack(float x,float y,float z,float rotY=0.0f,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,y,z); glRotatef(rotY,0,1,0); glScalef(scale,scale,scale);
    drawTexturedBox(0,0,0,3.65f,2.25f,0.16f,texWood,26);
    drawBox(0,0,0.10f,3.35f,1.95f,0.08f,0.15f,0.17f,0.13f,22,0.14f);
    // Three crossed/angled historic long-arm silhouettes as non-functional wall artifacts.
    for(float ox : {-0.95f,0.0f,0.95f}) {
        glPushMatrix();
        glTranslatef(ox,0,0.18f); glRotatef(-18.0f,0,0,1);
        drawBox(0,-0.48f,0,0.16f,0.56f,0.12f,0.28f,0.18f,0.10f,20,0.14f);
        drawCylinderBetween(Vec3(0,-0.28f,0),Vec3(0,0.72f,0),0.045f,0.10f,0.11f,0.09f);
        drawBox(0.11f,-0.04f,0,0.22f,0.16f,0.12f,0.12f,0.13f,0.10f,20,0.12f);
        glPopMatrix();
    }
    // Small brass title strip.
    drawBox(0,-0.86f,0.19f,2.45f,0.16f,0.05f,0.72f,0.54f,0.16f,44,0.54f);
    glPopMatrix();
}

void drawWallMedalBoard(float x,float y,float z,float rotY=0.0f,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,y,z); glRotatef(rotY,0,1,0); glScalef(scale,scale,scale);
    drawTexturedBox(0,0,0,3.20f,2.20f,0.15f,texWood,24);
    drawBox(0,0,0.10f,2.92f,1.92f,0.07f,0.12f,0.14f,0.12f,20,0.12f);
    for(int row=0;row<2;row++) for(int col=0;col<4;col++) {
        float px=-1.05f+col*0.70f, py=0.46f-row*0.72f;
        drawBox(px,py+0.24f,0.18f,0.08f,0.42f,0.04f,0.48f,0.08f,0.07f,30,0.18f);
        drawSphere(px,py,0.20f,0.13f,0.78f,0.60f,0.18f,48,0.58f);
    }
    glPopMatrix();
}

void drawBust(float x,float z,float scale=1.0f,float tone=0.72f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(scale,scale,scale);
    drawTexturedBox(0,0.65f,0,1.7f,1.3f,1.7f,texFloor,42);
    drawCylinderY(0,1.25f,0,0.45f,0.75f,tone,tone*0.98f,tone*0.95f,60,0.55f);
    drawScaledSphere(0,2.35f,0,0.52f,0.68f,0.48f,tone,tone*0.98f,tone*0.95f,60,0.55f);
    drawScaledSphere(0,1.90f,0,0.86f,0.38f,0.50f,tone,tone*0.98f,tone*0.95f,60,0.55f);
    drawScaledSphere(-0.17f,2.45f,0.44f,0.06f,0.045f,0.04f,0.09f,0.07f,0.05f,60,0.4f);
    drawScaledSphere( 0.17f,2.45f,0.44f,0.06f,0.045f,0.04f,0.09f,0.07f,0.05f,60,0.4f);
    glPopMatrix();
}

void drawVase(float x,float z,float scale,float r,float g,float b) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(scale,scale,scale);
    drawTexturedBox(0,0.45f,0,1.6f,0.9f,1.6f,texWood,30);
    drawScaledSphere(0,1.35f,0,0.55f,0.85f,0.55f,r,g,b,55,0.55f);
    drawCylinderY(0,1.80f,0,0.27f,0.52f,r*0.92f,g*0.92f,b*0.92f,55,0.55f);
    drawCylinderY(0,2.28f,0,0.39f,0.14f,r*0.85f,g*0.85f,b*0.85f,55,0.55f);
    glPopMatrix();
}

void drawWallMask(float x,float y,float z,float rotY,float r,float g,float b) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawTexturedBox(0,0,0,2.0f,2.8f,0.15f,texWood,30);
    glTranslatef(0,0,0.20f);
    drawScaledSphere(0,0,0,0.62f,0.92f,0.22f,r,g,b,55,0.55f);
    drawScaledSphere(-0.22f,0.18f,0.20f,0.09f,0.07f,0.05f,0.05f,0.04f,0.03f,55,0.4f);
    drawScaledSphere( 0.22f,0.18f,0.20f,0.09f,0.07f,0.05f,0.05f,0.04f,0.03f,55,0.4f);
    drawBox(0,-0.18f,0.24f,0.34f,0.07f,0.05f,0.20f,0.08f,0.04f,25,0.25f);
    glPopMatrix();
}

// Forward declarations for the illuminated entrance clock.
void resetEmission();
void drawGlowSphere(float x,float y,float z,float sx,float sy,float sz,
                    float r,float g,float b,float glow);


// Flat, unlit clock graphics keep the face and hands readable even when many
// OpenGL lights are active in the lobby.
void drawClockDisc(float z,float radius,float r,float g,float b) {
    glDisable(GL_LIGHTING);
    glColor3f(r,g,b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,0,z);
    for(int i=0;i<=64;i++) {
        float a=2.0f*PI*float(i)/64.0f;
        glVertex3f(radius*std::sin(a),radius*std::cos(a),z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawClockHand2D(float length,float width,float angleDeg,float z,float r,float g,float b) {
    glPushMatrix();
    glRotatef(angleDeg,0,0,1);
    glDisable(GL_LIGHTING);
    glColor3f(r,g,b);
    glBegin(GL_QUADS);
    glVertex3f(-width*0.5f,-0.06f,z);
    glVertex3f( width*0.5f,-0.06f,z);
    glVertex3f( width*0.5f,length,z);
    glVertex3f(-width*0.5f,length,z);
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawWallClock(float x,float y,float z,float rotY) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // Step 12: double-sided entrance clock with an unmistakable dark face,
    // thick bright hands and visible suspension from the entrance canopy.
    // The previous material-only face could become pale under multiple lights.
    drawCylinderY(-0.34f,0.78f,0,0.035f,3.70f,0.10f,0.075f,0.045f,32,0.20f);
    drawCylinderY( 0.34f,0.78f,0,0.035f,3.70f,0.10f,0.075f,0.045f,32,0.20f);
    drawBox(0,4.42f,0,1.08f,0.18f,0.22f,0.20f,0.13f,0.055f,48,0.42f);

    // Bronze rim and a dark backing make the silhouette readable from outside.
    setMaterial(0.76f,0.44f,0.08f,72,0.72f);
    glutSolidTorus(0.13f,0.82f,22,56);
    drawScaledSphere(0,0,0,0.78f,0.78f,0.10f,0.055f,0.070f,0.10f,18,0.02f);

    for(int side=0; side<2; ++side) {
        glPushMatrix();
        if(side==1) glRotatef(180.0f,0,1,0);

        // Unlit deep-blue face: it stays navy instead of turning white/beige
        // against the interior wall or strong museum spotlights.
        drawClockDisc(0.112f,0.675f,0.018f,0.055f,0.135f);

        // Bright gold hour dots.
        for(int i=0;i<12;i++) {
            float a=2.0f*PI*float(i)/12.0f;
            float px=0.535f*std::sin(a);
            float py=0.535f*std::cos(a);
            drawGlowSphere(px,py,0.145f,0.048f,0.048f,0.032f,
                           1.0f,0.73f,0.14f,0.78f);
        }

        // Two thick, high-contrast hands.  Their unlit colors guarantee that
        // they remain visible from both the exterior and the lobby.
        drawClockHand2D(0.34f,0.075f,-8.0f,0.165f,1.0f,0.78f,0.18f);
        drawClockHand2D(0.49f,0.052f,-52.0f,0.171f,0.92f,0.96f,1.0f);
        drawGlowSphere(0,0,0.185f,0.105f,0.105f,0.050f,1.0f,0.36f,0.06f,0.82f);

        glPopMatrix();
    }

    glPopMatrix();
}

void drawAccentPanel(float x,float y,float z,float sx,float sy,float sz,float r,float g,float b) {
    drawBox(x,y,z,sx,sy,sz,r,g,b,24,0.2f);
}

// ================================================================
// Museum architecture
// ================================================================

void drawLowGuideLights();

void drawColumn(float x,float z) {
    drawCylinderY(x,0.25f,z,0.65f,8.7f,0.82f,0.82f,0.80f,50,0.45f);
    drawBox(x,0.25f,z,1.7f,0.5f,1.7f,0.70f,0.70f,0.68f,38,0.4f);
    drawBox(x,8.95f,z,1.65f,0.5f,1.65f,0.70f,0.70f,0.68f,38,0.4f);
}

void drawPlant(float x,float z,float s=1.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(s,s,s);
    drawCylinderY(0,0,0,0.7f,0.65f,0.38f,0.20f,0.10f,22,0.25f);
    drawCylinderY(0,0.55f,0,0.12f,1.5f,0.25f,0.16f,0.08f,16,0.1f);
    drawScaledSphere(-0.45f,2.1f,0,0.62f,0.95f,0.35f,0.12f,0.42f,0.16f,20,0.1f);
    drawScaledSphere( 0.45f,2.2f,0,0.62f,0.95f,0.35f,0.10f,0.46f,0.17f,20,0.1f);
    drawScaledSphere(0,2.55f,0.18f,0.65f,0.90f,0.35f,0.14f,0.50f,0.20f,20,0.1f);
    glPopMatrix();
}

// Taller outdoor tree used beyond the forecourt.  Its canopy reaches the
// second-floor sightline so the outside environment is genuinely visible
// through the upper windows instead of the windows reading as dark panels.
void drawOutdoorTree(float x,float z,float s=1.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(s,s,s);

    drawCylinderY(0,0,0,0.30f,7.6f,0.34f,0.20f,0.09f,24,0.20f);
    drawCylinderBetween(Vec3(0,5.5f,0),Vec3(-1.30f,7.35f,0.20f),0.12f,0.28f,0.18f,0.08f);
    drawCylinderBetween(Vec3(0,5.9f,0),Vec3( 1.25f,7.65f,-0.10f),0.11f,0.28f,0.18f,0.08f);

    drawScaledSphere(-1.20f,8.10f, 0.10f,1.95f,1.45f,1.55f,0.08f,0.34f,0.12f,18,0.08f);
    drawScaledSphere( 1.15f,8.25f,-0.10f,2.00f,1.55f,1.55f,0.07f,0.38f,0.13f,18,0.08f);
    drawScaledSphere( 0.00f,9.45f, 0.05f,2.25f,1.70f,1.75f,0.09f,0.42f,0.14f,18,0.08f);
    drawScaledSphere( 0.10f,7.65f, 0.40f,2.35f,1.55f,1.65f,0.10f,0.36f,0.12f,18,0.08f);

    glPopMatrix();
}

void drawRopeBarrier(float x1,float z1,float x2,float z2) {
    drawCylinderY(x1,0,z1,0.10f,1.15f,0.55f,0.45f,0.12f,50,0.65f);
    drawCylinderY(x2,0,z2,0.10f,1.15f,0.55f,0.45f,0.12f,50,0.65f);
    Vec3 a(x1,0.95f,z1), b(x2,0.95f,z2);
    Vec3 mid=(a+b)*0.5f;
    mid.y-=0.20f;
    drawCylinderBetween(a,mid,0.045f,0.52f,0.03f,0.05f);
    drawCylinderBetween(mid,b,0.045f,0.52f,0.03f,0.05f);
}

void drawCeilingFan(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,8.8f,z);
    glScalef(scale,scale,scale);
    drawCylinderY(0,0,0,0.11f,0.65f,0.22f,0.22f,0.23f,28,0.4f);
    glTranslatef(0,0.60f,0);
    glRotatef(fanRotation,0,1,0);
    drawSphere(0,0,0,0.22f,0.18f,0.18f,0.20f,35,0.5f);
    for(int i=0;i<4;i++) {
        glPushMatrix();
        glRotatef(i*90.0f,0,1,0);
        drawBox(1.05f,0,0,1.9f,0.10f,0.30f,0.33f,0.24f,0.15f,24,0.22f);
        glPopMatrix();
    }
    glPopMatrix();
}


// ================================================================
// Step 3: Interior atmosphere - floor, ceiling and architectural trim
// ================================================================

void resetEmission() {
    GLfloat e[] = {0.0f,0.0f,0.0f,1.0f};
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,e);
}

void drawGlowSphere(float x,float y,float z,float sx,float sy,float sz,
                    float r,float g,float b,float glow=0.34f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterial(r,g,b,72,0.75f);
    GLfloat emission[] = {r*glow,g*glow,b*glow,1.0f};
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emission);
    glutSolidSphere(1.0,20,12);
    resetEmission();
    glPopMatrix();
}

void drawFloorInlayRect(float cx,float cz,float w,float d,
                        float r=0.55f,float g=0.42f,float b=0.16f) {
    const float y=0.055f;
    const float t=0.10f;
    drawBox(cx,y,cz-d*0.5f,w,t,0.12f,r,g,b,55,0.48f);
    drawBox(cx,y,cz+d*0.5f,w,t,0.12f,r,g,b,55,0.48f);
    drawBox(cx-w*0.5f,y,cz,0.12f,t,d,r,g,b,55,0.48f);
    drawBox(cx+w*0.5f,y,cz,0.12f,t,d,r,g,b,55,0.48f);
}

void drawMuseumFloorDetails() {
    // A thin double-line inlay visually guides visitors through the main axis.
    drawBox(-3.85f,0.058f,0,0.10f,0.075f,48.0f,0.58f,0.43f,0.16f,55,0.50f);
    drawBox( 3.85f,0.058f,0,0.10f,0.075f,48.0f,0.58f,0.43f,0.16f,55,0.50f);

    // Room-by-room borders create the polished gallery-floor feeling.
    drawFloorInlayRect(-15.0f, 16.0f,15.0f,13.0f);
    drawFloorInlayRect( 15.0f, 16.0f,15.0f,13.0f);
    drawFloorInlayRect(-15.0f, -1.0f,15.0f,14.0f,0.34f,0.42f,0.58f);
    drawFloorInlayRect( 15.0f, -1.0f,15.0f,14.0f,0.30f,0.50f,0.35f);
    drawFloorInlayRect(-15.0f,-17.0f,15.0f,12.0f,0.60f,0.43f,0.17f);
    drawFloorInlayRect( 15.0f,-17.0f,15.0f,12.0f,0.58f,0.42f,0.16f);
    drawFloorInlayRect(  0.0f,-30.0f,34.0f,10.0f,0.48f,0.48f,0.46f);

    // The two large floor rosettes near the lobby/entry were visually
    // distracting and are intentionally removed. The floor remains clean.
}

void drawCeilingCoffer(float cx,float cz,float w,float d,
                       float r=0.52f,float g=0.46f,float b=0.34f) {
    const float y=9.32f;
    const float t=0.20f;
    drawBox(cx,y,cz-d*0.5f,w,t,0.26f,r,g,b,35,0.35f);
    drawBox(cx,y,cz+d*0.5f,w,t,0.26f,r,g,b,35,0.35f);
    drawBox(cx-w*0.5f,y,cz,0.26f,t,d,r,g,b,35,0.35f);
    drawBox(cx+w*0.5f,y,cz,0.26f,t,d,r,g,b,35,0.35f);

    // softly glowing central recessed fixture
    drawBox(cx,9.39f,cz,w*0.50f,0.10f,d*0.46f,0.90f,0.89f,0.84f,18,0.12f);
    drawGlowSphere(cx,9.19f,cz,0.38f,0.09f,0.38f,1.0f,0.87f,0.62f,0.28f);
}

void drawCeilingDetails() {
    // Lobby receives the richest ceiling composition.
    drawCeilingCoffer(0,29.0f,12.0f,10.0f,0.55f,0.43f,0.20f);

    // Main corridor: repeated bays create depth as the visitor walks forward.
    for(float z: {21.5f,12.0f,3.5f,-5.0f,-13.5f,-22.0f})
        drawCeilingCoffer(0,z,7.0f,6.0f,0.45f,0.40f,0.32f);

    // Side galleries use restrained rectangular coffers.
    drawCeilingCoffer(-15.0f,16.0f,13.5f,10.5f,0.34f,0.39f,0.32f);
    drawCeilingCoffer( 15.0f,16.0f,13.5f,10.5f,0.45f,0.37f,0.27f);
    drawCeilingCoffer(-15.0f,-1.0f,13.5f,11.0f,0.28f,0.31f,0.46f);
    drawCeilingCoffer( 15.0f,-1.0f,13.5f,11.0f,0.30f,0.43f,0.31f);
    drawCeilingCoffer(-15.0f,-17.0f,13.5f,9.5f,0.48f,0.37f,0.22f);
    drawCeilingCoffer( 15.0f,-17.0f,13.5f,9.5f,0.43f,0.35f,0.24f);
    drawCeilingCoffer(0,-30.0f,30.0f,8.0f,0.43f,0.43f,0.41f);
}

void drawWallWainscoting() {
    // Dark lower-wall panels add visual weight and make the galleries feel
    // more like finished interiors instead of large plain boxes.
    const float y=1.25f;
    const float h=2.25f;
    const float t=0.10f;

    // Main corridor wall bases.
    drawBox(-5.73f,y, 7.5f,t,h,12.0f,0.20f,0.15f,0.10f,30,0.22f);
    drawBox( 5.73f,y, 7.5f,t,h,12.0f,0.20f,0.15f,0.10f,30,0.22f);
    drawBox(-5.73f,y,-9.0f,t,h,11.0f,0.20f,0.15f,0.10f,30,0.22f);
    drawBox( 5.73f,y,-9.0f,t,h,11.0f,0.20f,0.15f,0.10f,30,0.22f);

    // Outer walls in each side wing. Small depth avoids covering the art.
    drawBox(-23.72f,y,16.0f,t,h,13.0f,0.16f,0.13f,0.10f,28,0.18f);
    drawBox( 23.72f,y,16.0f,t,h,13.0f,0.16f,0.13f,0.10f,28,0.18f);
    drawBox(-23.72f,y,-1.0f,t,h,14.5f,0.15f,0.14f,0.16f,28,0.18f);
    drawBox( 23.72f,y,-1.0f,t,h,14.5f,0.14f,0.16f,0.14f,28,0.18f);
    drawBox(-23.72f,y,-17.0f,t,h,12.0f,0.17f,0.13f,0.09f,28,0.18f);
    drawBox( 23.72f,y,-17.0f,t,h,12.0f,0.17f,0.13f,0.09f,28,0.18f);

    // Brass chair-rail line above the wood panels.
    drawBox(-23.61f,2.42f, 16.0f,0.05f,0.09f,13.0f,0.62f,0.45f,0.17f,52,0.55f);
    drawBox( 23.61f,2.42f, 16.0f,0.05f,0.09f,13.0f,0.62f,0.45f,0.17f,52,0.55f);
    drawBox(-23.61f,2.42f, -1.0f,0.05f,0.09f,14.5f,0.50f,0.47f,0.38f,52,0.50f);
    drawBox( 23.61f,2.42f, -1.0f,0.05f,0.09f,14.5f,0.50f,0.47f,0.38f,52,0.50f);
    drawBox(-23.61f,2.42f,-17.0f,0.05f,0.09f,12.0f,0.62f,0.45f,0.17f,52,0.55f);
    drawBox( 23.61f,2.42f,-17.0f,0.05f,0.09f,12.0f,0.62f,0.45f,0.17f,52,0.55f);
}

void drawAtmosphereFixtures() {
    // Additional ceiling fixtures line up with the real OpenGL light positions.
    const Vec3 fixtures[] = {
        Vec3(-15.0f,9.13f,16.0f), Vec3(15.0f,9.13f,16.0f),
        Vec3(-15.0f,9.13f,-1.0f), Vec3(15.0f,9.13f,-1.0f),
        Vec3(-15.0f,9.13f,-17.0f), Vec3(15.0f,9.13f,-17.0f),
        Vec3(0.0f,9.13f,-30.0f)
    };
    for(const Vec3& q:fixtures) {
        drawBox(q.x,9.28f,q.z,1.15f,0.12f,1.15f,0.34f,0.31f,0.26f,38,0.36f);
        drawGlowSphere(q.x,q.y,q.z,0.34f,0.08f,0.34f,1.0f,0.86f,0.58f,0.30f);
    }
}


// ================================================================
// Step 10A: Grand staircase + second-floor structural shell
// ================================================================

void drawUpperWindow(float x,float y,float z) {
    // REAL WINDOW OPENING: there is deliberately no opaque box behind this
    // frame.  The segmented front wall leaves an actual hole, so the exterior
    // forecourt, trees and distant landscape are visible from inside Level 2.
    const float w=4.50f, h=4.55f;
    const float frame=0.18f;

    // Warm bronze/wood surround.
    drawTexturedBox(x, y+h*0.5f, z, w+0.28f,frame,0.24f,texWood,30);
    drawTexturedBox(x, y-h*0.5f, z, w+0.28f,frame,0.24f,texWood,30);
    drawTexturedBox(x-w*0.5f, y, z, frame,h,0.24f,texWood,30);
    drawTexturedBox(x+w*0.5f, y, z, frame,h,0.24f,texWood,30);

    // Mullions. They frame the view without covering it.
    drawBox(x,y,z+0.03f,0.10f,h-0.24f,0.10f,0.58f,0.42f,0.14f,55,0.58f);
    drawBox(x,y,z+0.03f,w-0.24f,0.10f,0.10f,0.58f,0.42f,0.14f,55,0.58f);

    // Two very thin glass-highlight strips suggest glazing while keeping the
    // opening optically clear. No full-size pane is drawn, so the outside view
    // remains visible on fixed-function OpenGL setups without alpha sorting.
    drawBox(x-w*0.28f,y+h*0.23f,z+0.08f,0.055f,h*0.34f,0.025f,
            0.58f,0.76f,0.88f,80,0.72f);
    drawBox(x+w*0.23f,y-h*0.20f,z+0.08f,0.045f,h*0.28f,0.025f,
            0.58f,0.76f,0.88f,80,0.72f);
}

// Front wall for Level 2, split into masonry bands and piers around six
// genuine window openings.  This directly satisfies the assignment's
// "external environment visible through a window" requirement.
void drawUpperFrontWallWithOpenWindows() {
    const float z=36.0f;

    // Solid wall below and above the window row.
    drawTexturedBox(0,11.10f,z,48.0f,2.20f,0.50f,texWall,10); // y 10.00..12.20
    drawTexturedBox(0,18.25f,z,48.0f,2.50f,0.50f,texWall,10); // y 17.00..19.50

    // Vertical wall piers between the six 4.5-unit openings.
    const float yc=14.60f, h=4.80f;
    const float centers[] = {-22.125f,-14.50f,-7.50f,0.0f,7.50f,14.50f,22.125f};
    const float widths[]  = {  3.75f,   2.50f, 2.50f,3.50f,2.50f, 2.50f, 3.75f};
    for(int i=0;i<7;i++)
        drawTexturedBox(centers[i],yc,z,widths[i],h,0.50f,texWall,10);

    // Window frames sit just outside the wall plane.
    for(float x : {-18.0f,-11.0f,-4.0f,4.0f,11.0f,18.0f})
        drawUpperWindow(x,14.60f,36.30f);
}

void drawRailingPost(float x,float y,float z,float h=1.22f) {
    drawCylinderY(x,y,z,0.055f,h,0.38f,0.27f,0.10f,55,0.60f);
    drawSphere(x,y+h,z,0.095f,0.72f,0.52f,0.16f,70,0.72f);
}

void drawHorizontalRailingZ(float x,float z1,float z2,float y) {
    if(z2<z1) std::swap(z1,z2);
    for(float z=z1; z<=z2+0.01f; z+=1.25f) drawRailingPost(x,y,z);
    drawCylinderBetween(Vec3(x,y+1.22f,z1),Vec3(x,y+1.22f,z2),0.055f,0.68f,0.48f,0.14f);
    drawCylinderBetween(Vec3(x,y+0.62f,z1),Vec3(x,y+0.62f,z2),0.035f,0.48f,0.34f,0.12f);
}

void drawHorizontalRailingX(float x1,float x2,float z,float y) {
    if(x2<x1) std::swap(x1,x2);
    for(float x=x1; x<=x2+0.01f; x+=1.25f) drawRailingPost(x,y,z);
    drawCylinderBetween(Vec3(x1,y+1.22f,z),Vec3(x2,y+1.22f,z),0.055f,0.68f,0.48f,0.14f);
    drawCylinderBetween(Vec3(x1,y+0.62f,z),Vec3(x2,y+0.62f,z),0.035f,0.48f,0.34f,0.12f);
}

void drawSlopedStairRail(float x,float zStart,float yStart,float zEnd,float yEnd) {
    // More regular posts make the railing read as one continuous handrail.
    const int posts=8;
    for(int i=0;i<=posts;i++) {
        float t=float(i)/float(posts);
        float z=zStart+(zEnd-zStart)*t;
        float y=yStart+(yEnd-yStart)*t;
        drawRailingPost(x,y,z,1.10f);
    }
    drawCylinderBetween(Vec3(x,yStart+1.10f,zStart),Vec3(x,yEnd+1.10f,zEnd),0.060f,0.70f,0.50f,0.15f);
    drawCylinderBetween(Vec3(x,yStart+0.58f,zStart),Vec3(x,yEnd+0.58f,zEnd),0.035f,0.48f,0.34f,0.12f);
}

void drawGrandStaircase() {
    // 16 wider steps per flight: cleaner proportions and fewer draw calls.
    const int N=16;
    const float run=STAIR_Z_FRONT-STAIR_Z_REAR;
    const float tread=run/float(N);
    const float rise=STAIR_MID_Y/float(N);

    for(int i=0;i<N;i++) {
        float top=(i+1)*rise;
        float z=STAIR_Z_FRONT-(i+0.5f)*tread;
        drawTexturedBox(STAIR_FIRST_X,top-0.10f,z,STAIR_FLIGHT_W,0.20f,tread+0.04f,texFloor,34);
        drawBox(STAIR_FIRST_X,top-0.205f,z,STAIR_FLIGHT_W+0.10f,0.055f,tread+0.05f,
                0.62f,0.45f,0.16f,55,0.58f);
    }

    const float landingX=(STAIR_LANDING_X_MIN+STAIR_LANDING_X_MAX)*0.5f;
    const float landingZ=(STAIR_LANDING_Z_MIN+STAIR_LANDING_Z_MAX)*0.5f;
    const float landingW=STAIR_LANDING_X_MAX-STAIR_LANDING_X_MIN;
    const float landingD=STAIR_LANDING_Z_MAX-STAIR_LANDING_Z_MIN;
    drawTexturedBox(landingX,STAIR_MID_Y-0.12f,landingZ,
                    landingW,0.24f,landingD,texFloor,36);
    drawBox(landingX,STAIR_MID_Y-0.255f,landingZ,
            landingW+0.10f,0.055f,landingD+0.10f,
            0.62f,0.45f,0.16f,55,0.58f);

    for(int i=0;i<N;i++) {
        float top=STAIR_MID_Y+(i+1)*rise;
        float z=STAIR_Z_REAR+(i+0.5f)*tread;
        drawTexturedBox(STAIR_SECOND_X,(STAIR_MID_Y+top)*0.5f,z,
                        STAIR_FLIGHT_W,top-STAIR_MID_Y,tread+0.04f,texFloor,34);
        drawBox(STAIR_SECOND_X,top-0.205f,z,STAIR_FLIGHT_W+0.10f,0.055f,tread+0.05f,
                0.62f,0.45f,0.16f,55,0.58f);
    }

    const float topX=(15.35f+23.45f)*0.5f;
    const float topZ=(STAIR_TOP_Z_MIN+STAIR_TOP_Z_MAX)*0.5f;
    drawTexturedBox(topX,SECOND_FLOOR_Y-0.12f,topZ,
                    8.10f,0.24f,STAIR_TOP_Z_MAX-STAIR_TOP_Z_MIN,texFloor,36);
    drawBox(topX,SECOND_FLOOR_Y-0.255f,topZ,
            8.20f,0.055f,(STAIR_TOP_Z_MAX-STAIR_TOP_Z_MIN)+0.10f,
            0.62f,0.45f,0.16f,55,0.58f);

    float half=STAIR_FLIGHT_W*0.5f+0.10f;
    drawSlopedStairRail(STAIR_FIRST_X-half,STAIR_Z_FRONT,0.0f,STAIR_Z_REAR,STAIR_MID_Y);
    drawSlopedStairRail(STAIR_FIRST_X+half,STAIR_Z_FRONT,0.0f,STAIR_Z_REAR,STAIR_MID_Y);
    drawSlopedStairRail(STAIR_SECOND_X-half,STAIR_Z_REAR,STAIR_MID_Y,STAIR_Z_FRONT,SECOND_FLOOR_Y);
    drawSlopedStairRail(STAIR_SECOND_X+half,STAIR_Z_REAR,STAIR_MID_Y,STAIR_Z_FRONT,SECOND_FLOOR_Y);

    // Short landing guards improve safety while leaving both stair approaches open.
    // Nothing spans the actual second-floor stair exit.
    drawHorizontalRailingZ(15.55f,25.20f,26.20f,STAIR_MID_Y);
    drawHorizontalRailingZ(23.25f,25.20f,26.20f,STAIR_MID_Y);
    drawHorizontalRailingX(15.55f,16.65f,25.20f,STAIR_MID_Y);
    drawHorizontalRailingX(22.15f,23.25f,25.20f,STAIR_MID_Y);

    drawRoomSign("GRAND STAIRCASE / LEVEL 2",23.60f,6.70f,30.0f,-90,5.7f);

    for(int i=0;i<8;i++) {
        float t=float(i)/7.0f;
        float z=STAIR_Z_FRONT+(STAIR_Z_REAR-STAIR_Z_FRONT)*t;
        float y=STAIR_MID_Y*t+0.18f;
        drawGlowSphere(STAIR_FIRST_X-half-0.16f,y,z,0.055f,0.055f,0.055f,
                       1.0f,0.58f,0.16f,0.62f);
    }
}

void drawSecondFloorShell() {
    // The old ground-floor ceiling becomes the structural slab of level 2.
    // It is split to leave a real opening over the grand staircase.
    const float slabY=9.72f;
    drawTexturedBox(-4.45f,slabY,0.0f,39.10f,0.34f,72.0f,texCeiling,12);      // large left/main slab
    drawTexturedBox(19.65f,slabY,-5.45f,8.70f,0.34f,60.90f,texCeiling,12);    // right rear slab
    drawTexturedBox(19.40f,slabY,34.20f,8.10f,0.34f,2.50f,texCeiling,12);     // broad upper landing

    // Step 10B: Level 2 stays as ONE large open-plan common exhibition hall.
    // The carpet is a visual promenade only; there are no room-dividing walls upstairs.
    drawTexturedBox(0.0f,SECOND_FLOOR_Y+0.025f,-1.5f,7.2f,0.07f,61.0f,texCarpet,10);
    drawTexturedBox(8.0f,SECOND_FLOOR_Y+0.025f,34.85f,16.0f,0.07f,1.05f,texCarpet,10);

    // Upper exterior shell.  Ground-floor architecture remains untouched.
    drawTexturedBox(-24.0f,14.75f,0,0.50f,9.50f,72.0f,texWall,10);
    drawTexturedBox( 24.0f,14.75f,0,0.50f,9.50f,72.0f,texWall,10);
    drawTexturedBox(0,14.75f,-36.0f,48.0f,9.50f,0.50f,texWall,10);
    drawUpperFrontWallWithOpenWindows();

    // Second-floor roof and exterior crown line.
    drawTexturedBox(0,19.58f,0,48.0f,0.34f,72.0f,texCeiling,10);
    drawBox(0,19.30f,35.68f,47.5f,0.34f,0.22f,0.58f,0.43f,0.14f,55,0.65f);
    drawBox(0,10.15f,35.70f,47.5f,0.22f,0.18f,0.58f,0.43f,0.14f,55,0.65f);

    // Stairwell opening railings on level 2.
    // The long front rail across z=25.15 used to read as an extra pipe
    // blocking the stair exit, so that element is intentionally removed.
    drawHorizontalRailingZ(15.35f,25.15f,32.65f,SECOND_FLOOR_Y);
    drawHorizontalRailingX(15.35f,19.45f,32.65f,SECOND_FLOOR_Y);
    drawHorizontalRailingX(22.55f,23.45f,32.65f,SECOND_FLOOR_Y);

    // Upper landing details: sign, decorative lamps and two plants.  No benches
    // are placed in the stair circulation zone.
    drawRoomSign("LEVEL 2 - LIBERATION WAR EXHIBITION",0.0f,16.80f,35.62f,180,10.5f);
    for(float x : {-9.0f,9.0f}) drawWallLamp(x,14.70f,35.55f,180);
    glPushMatrix();
    glTranslatef(0,SECOND_FLOOR_Y,0);
    drawPlant(-13.5f,33.8f,0.62f);
    drawPlant( 12.8f,33.8f,0.62f);
    glPopMatrix();

}

// ================================================================
// Level 2 theme replacement: Liberation War Exhibition (1971)

// Forward declarations for themed display elements used before their definitions.
void drawGlassCase(float x, float z, int artifactType, float rotY);
void drawMuseumVisitor(float x,float z,float scale,float rotY);
void drawFreedomFighterMannequin(float x,float z,float scale,float rotY);
void drawBangladeshFlag(float x,float z,float poleH,float scale);

// First-floor Liberation War displays and memorials.
// These functions are defined later in the source file, so declare them here.
void drawPrintingPressDisplay(float x,float z);
void drawRadioDeskDisplay(float x,float z);
void drawRefugeeReliefDisplay(float x,float z);
void drawDocumentArchiveTable(float x,float z,float rotY=0.0f);
void drawResistanceCampDisplay(float x,float z);
void drawVictoryArchiveDisplay(float x,float z);
void drawFieldWeaponsDisplay(float x,float z);
void drawFieldMedicalUnitDisplay(float x,float z,float rotY=0.0f);
void drawSevenMarchMemorial(float x,float z,float scale);
void drawFreedomFighterStatue(float x,float z,float scale);
void drawMotherChildMemorial(float x,float z,float scale);
void drawVictoryFlagMemorial(float x,float z,float scale);
void drawFreedomFighterGroupMemorial(float x,float z,float scale);
void drawFieldNurseStatue(float x,float z,float scale,float rotY=0.0f);
void drawShaheedMinarMemorial(float x,float z,float scale,float rotY=0.0f);
void drawRadioOperatorMemorial(float x,float z,float scale);
void drawTransformedMemorial(int id, void (*drawMemorial)(float,float,float),
                             float x,float z,float baseScale);

void drawUpperCommonHall() {
    glPushMatrix();
    glTranslatef(0,SECOND_FLOOR_Y,0);

    // ============================================================
    // LEVEL 2: BANGLADESH LIBERATION WAR EXHIBITION, 1971
    // One open-plan historical exhibition with a clear visitor loop.
    // ============================================================

    // Arrival wall: title + museum information + Bangladesh map.
    drawCollectionIntroPanel("LIBERATION WAR OF BANGLADESH",
                             "1971  |  PEOPLE  |  RESISTANCE  |  VICTORY",
                             -9.0f,5.15f,35.58f,180,10.2f);
    drawRoomSign("MUSEUM INFO",0.0f,5.35f,35.35f,180,4.45f);
    drawWallArtwork(0.0f,3.15f,35.40f,4.80f,2.85f,texWarMap,180,
                    "BANGLADESH - 1971","EDUCATIONAL MAP • SECTORS / RIVERS / WAR CONTEXT",false);

    // Left wall: historical scenes.
    drawWallArtwork(-23.64f,5.45f,25.5f,4.05f,2.95f,texWarPhoto1,90,
                    "MASS MOVEMENT","HISTORICAL SCENE • 1971",true);
    drawWallArtwork(-23.64f,5.45f,13.0f,4.05f,2.95f,texWarPhoto2,90,
                    "FREEDOM FIGHTERS","HISTORICAL SCENE • 1971",true);
    drawWallArtwork(-23.64f,5.45f,0.0f,4.05f,2.95f,texWarPhoto3,90,
                    "RIVER CROSSING","HISTORICAL SCENE • 1971",true);
    drawWallArtwork(-23.64f,5.45f,-13.0f,4.05f,2.95f,texWarPhoto4,90,
                    "DISPLACEMENT & REFUGEES","CIVILIAN EXPERIENCE • 1971",true);
    drawWallArtwork(-23.64f,5.45f,-26.0f,4.05f,2.95f,texWarPhoto5,90,
                    "VICTORY GATHERING","HISTORICAL SCENE • DECEMBER 1971",true);

    // Right wall: complementary scenes + documentation.
    drawWallArtwork(23.64f,5.45f,24.0f,4.05f,2.95f,texWarPhoto6,-90,
                    "DOCUMENTS & RADIO","ARCHIVE DISPLAY • 1971",true);
    drawWallArtwork(23.64f,5.45f,11.0f,4.05f,2.95f,texWarPhoto1,-90,
                    "PEOPLE IN RESISTANCE","HISTORICAL SCENE • 1971",true);
    drawWallArtwork(23.64f,5.45f,-2.0f,4.05f,2.95f,texWarPhoto2,-90,
                    "GUERRILLA MEMORY","HISTORICAL SCENE • 1971",true);
    drawWallArtwork(23.64f,5.45f,-15.0f,4.05f,2.95f,texWarPhoto3,-90,
                    "RIVER & COMMUNICATION","HISTORICAL SCENE • 1971",true);
    drawWallArtwork(23.64f,5.45f,-28.0f,4.05f,2.95f,texWarPhoto5,-90,
                    "VICTORY MEMORY","HISTORICAL SCENE • 1971",true);

    // Rear wall: a compact timeline / context panel and two photographs.
    drawWallArtwork(-13.8f,5.30f,-35.58f,4.15f,2.95f,texWarPhoto4,0,
                    "DISPLACED COMMUNITIES","CIVILIAN EXPERIENCE • 1971",true);
    drawWallArtwork( 13.8f,5.30f,-35.58f,4.15f,2.95f,texWarPhoto6,0,
                    "ARCHIVE & COMMUNICATION","DOCUMENTS / RADIO",true);
    drawCollectionIntroPanel("1971 TIMELINE",
                             "26 MAR  INDEPENDENCE  •  10 APR  GOVERNMENT  •  16 DEC  VICTORY",
                             0.0f,7.35f,-35.60f,0,10.6f);


    // High wall artifact panels fill the unused band above normal eye-level exhibits.
    drawWallWeaponRack(-23.60f,8.00f,6.5f, 90.0f,0.66f);
    drawWallMedalBoard(-23.60f,8.00f,-6.5f,90.0f,0.66f);
    drawWallMedalBoard( 23.60f,8.00f,5.0f,-90.0f,0.66f);
    drawWallWeaponRack(23.60f,8.00f,-8.0f,-90.0f,0.66f);

    // ---------------- Floor artifacts: war-only object cases.
    // Compact layout keeps the central visitor route open.
    drawGlassCase(-10.0f,17.0f,3,0);  // rifle
    drawGlassCase( 10.0f,17.0f,4,0);  // field radio
    drawGlassCase(-10.0f, 5.5f,5,0);  // helmet
    drawGlassCase( 10.0f, 5.5f,6,0);  // pistol
    drawGlassCase(-10.0f,-8.0f,7,0);  // wartime documents
    drawGlassCase( 10.0f,-8.0f,8,0);  // medical kit
    drawGlassCase(-10.0f,-21.0f,9,0);  // field rucksack
    drawGlassCase( 10.0f,-21.0f,10,0); // binoculars

    // Additional side-gallery cases fill previously empty floor zones while
    // leaving the central visitor loop clear.
    drawGlassCase(-18.0f, 10.5f,5,90);   // helmet / personal equipment
    drawGlassCase( 18.0f, 10.5f,10,-90); // binoculars / observation
    drawGlassCase(-18.0f,-16.5f,7,90);   // archival documents
    drawGlassCase( 18.0f,-16.5f,9,-90);  // field rucksack


    // Major equipment gallery at the rear. These large museum models fill the
    // widest empty zone while leaving an open lane between them.
    drawArtilleryCannonDisplay(-15.5f,-28.4f,0.72f,8.0f);
    drawArtworkLabel("1971 ARTILLERY CANNON - DISPLAY MODEL",-15.5f,0.66f,-24.95f,180);
    draw1971MilitaryJeepDisplay(15.0f,-28.2f,0.76f,180.0f);
    drawArtworkLabel("PAKISTANI FORCES VEHICLE - DISPLAY MODEL",15.0f,0.66f,-24.80f,180);

    // Smaller supporting dioramas occupy side-wall pockets.
    drawMortarAndCrateDisplay(-18.0f,22.0f,0.70f,90.0f);
    drawArtworkLabel("MORTAR & SUPPLY DISPLAY",-16.35f,0.66f,22.0f,90);
    drawFieldCommandPostDiorama(17.6f,0.0f,0.68f,-90.0f);
    drawArtworkLabel("FIELD COMMAND POST",16.05f,0.66f,0.0f,-90);

    drawArtworkLabel("HISTORICAL RIFLE",-10.0f,0.62f,18.30f,180);
    drawArtworkLabel("FIELD RADIO",10.0f,0.62f,18.30f,180);
    drawArtworkLabel("FIELD HELMET",-10.0f,0.62f,6.80f,180);
    drawArtworkLabel("HISTORICAL PISTOL",10.0f,0.62f,6.80f,180);
    drawArtworkLabel("WAR DOCUMENTS",-10.0f,0.62f,-6.70f,180);
    drawArtworkLabel("MEDICAL KIT",10.0f,0.62f,-6.70f,180);
    drawArtworkLabel("FIELD RUCKSACK",-10.0f,0.62f,-19.70f,180);
    drawArtworkLabel("BINOCULARS",10.0f,0.62f,-19.70f,180);
    drawArtworkLabel("PERSONAL FIELD GEAR",-16.35f,0.62f,10.5f,90);
    drawArtworkLabel("OBSERVATION EQUIPMENT",16.35f,0.62f,10.5f,-90);
    drawArtworkLabel("ARCHIVE DOCUMENTS",-16.35f,0.62f,-16.5f,90);
    drawArtworkLabel("FIELD RUCKSACK",16.35f,0.62f,-16.5f,-90);

    // Compact secondary map on the rear wall: reinforces geography without
    // filling the floor with extra objects.
    drawWallArtwork(0.0f,4.55f,-35.58f,5.00f,2.65f,texWarMapDetail,0,
                    "LIBERATION WAR MAP","SECTOR NETWORK / RIVER CORRIDORS",false);

    // A compact eye-level kinetic centerpiece makes the continuous-rotation
    // requirement immediately obvious without blocking circulation.
    drawRotatingVictoryEmblem(0.0f,-11.8f,0.88f);
    drawArtworkLabel("WAR ARTIFACTS & DOCUMENTS",0.0f,0.78f,12.20f,180);

    // More visitors make the large hall feel active and give a better sense of scale.
    drawMuseumVisitor(-7.2f,26.0f,0.90f,165);
    drawMuseumVisitor( 7.4f,24.0f,0.88f,-145);
    drawMuseumVisitor(-12.8f,22.2f,0.84f,145);
    drawMuseumVisitor( 16.0f,18.0f,0.86f,-105);
    drawMuseumVisitor(-15.0f,-3.5f,0.82f,70);
    drawMuseumVisitor( 15.2f,-5.0f,0.88f,-70);
    drawMuseumVisitor(-7.8f,-30.5f,0.84f,25);
    drawMuseumVisitor( 7.5f,-31.0f,0.86f,-20);
    drawMuseumVisitor(-5.8f,12.8f,0.83f,170);
    drawMuseumVisitor( 6.2f,10.0f,0.86f,-165);
    drawMuseumVisitor(-19.0f,5.5f,0.80f,95);
    drawMuseumVisitor( 19.1f,-8.0f,0.82f,-90);
    drawMuseumVisitor(-8.0f,-18.0f,0.85f,25);
    drawMuseumVisitor( 8.2f,-16.0f,0.84f,-30);
    drawFreedomFighterMannequin(-4.9f,-2.6f,0.92f,180);
    drawArtworkLabel("FREEDOM FIGHTER",-4.9f,0.78f,-4.15f,180);

    // Minimal greenery / seating: just one bench at the rear edge.
    // Rear bench removed to keep the Level-2 war exhibition uncluttered.

    // Lighting is quieter than the old abstract-art hall so the archival panels read clearly.
    drawWallLamp(-6.5f,7.95f,30.0f,180);
    drawWallLamp( 6.5f,7.95f,30.0f,180);
    drawChandelier(0.0f,12.0f,0.78f);

    // Four subtle floor guide lights define the visitor loop without clutter.
    for(float z : {22.0f,-4.0f,-20.0f}){
        drawGlowSphere(-5.8f,0.28f,z,0.10f,0.05f,0.10f,0.92f,0.62f,0.20f,0.32f);
        drawGlowSphere( 5.8f,0.28f,z,0.10f,0.05f,0.10f,0.92f,0.62f,0.20f,0.32f);
    }

    glPopMatrix();
}

void drawMuseumArchitecture() {
    // ---------------- Main building shell ----------------
    drawTexturedBox(0,-0.25f,0,48.0f,0.5f,72.0f,texFloor,42);
    drawSecondFloorShell();
    drawGrandStaircase();
    drawUpperCommonHall();

    // Central corridor and lobby carpets
    drawTexturedBox(0,0.02f,0,8.5f,0.07f,48.0f,texCarpet,10);
    drawTexturedBox(0,0.025f,29.0f,12.0f,0.075f,12.0f,texCarpet,10);

    // Step 3 polish: marble inlays, architectural wall base and ceiling detail.
    drawMuseumFloorDetails();
    drawWallWainscoting();
    drawCeilingDetails();
    drawAtmosphereFixtures();

    // Outer walls
    drawTexturedBox(-24.0f,4.75f,0,0.50f,9.5f,72.0f,texWall,10);
    drawTexturedBox( 24.0f,4.75f,0,0.50f,9.5f,72.0f,texWall,10);
    drawTexturedBox(0,4.75f,-36.0f,48.0f,9.5f,0.50f,texWall,10);

    // Front facade with central entrance opening
    drawTexturedBox(-14.25f,4.75f,36.0f,19.5f,9.5f,0.50f,texWall,10);
    drawTexturedBox( 14.25f,4.75f,36.0f,19.5f,9.5f,0.50f,texWall,10);
    drawTexturedBox(0,8.30f,36.0f,9.0f,2.40f,0.50f,texWall,10);
    drawBox(0,9.15f,35.68f,47.5f,0.30f,0.22f,0.58f,0.43f,0.14f,55,0.65f);

    // Corridor side walls, deliberately split to create 3 doorways per side
    const float sideX[2]={-6.0f,6.0f};
    for(int i=0;i<2;i++){
        float x=sideX[i];
        drawTexturedBox(x,4.55f,21.25f,0.45f,9.1f,5.5f,texWall,10);
        drawTexturedBox(x,4.55f, 7.50f,0.45f,9.1f,12.0f,texWall,10);
        drawTexturedBox(x,4.55f,-9.00f,0.45f,9.1f,11.0f,texWall,10);
        drawTexturedBox(x,4.55f,-21.75f,0.45f,9.1f,4.5f,texWall,10);
        // Step 12: doorway trim objects were removed. From oblique views their
        // long lintels looked like unsupported floating slabs. The wall gaps
        // themselves now form clean, open museum entrances.
    }

    // Side-wing room separators: 3 rooms on each side
    drawTexturedBox(-15.0f,4.55f, 8.0f,18.0f,9.1f,0.45f,texWall,10);
    drawTexturedBox( 15.0f,4.55f, 8.0f,18.0f,9.1f,0.45f,texWall,10);
    drawTexturedBox(-15.0f,4.55f,-10.0f,18.0f,9.1f,0.45f,texWall,10);
    drawTexturedBox( 15.0f,4.55f,-10.0f,18.0f,9.1f,0.45f,texWall,10);

    // Decorative base / crown trims throughout corridor
    drawBox(-5.72f,0.32f,0,0.10f,0.55f,48.0f,0.36f,0.24f,0.10f,32,0.3f);
    drawBox( 5.72f,0.32f,0,0.10f,0.55f,48.0f,0.36f,0.24f,0.10f,32,0.3f);
    drawBox(-5.72f,8.82f,0,0.10f,0.28f,48.0f,0.55f,0.40f,0.12f,45,0.5f);
    drawBox( 5.72f,8.82f,0,0.10f,0.28f,48.0f,0.55f,0.40f,0.12f,45,0.5f);

    // ---------------- Lobby ----------------
    // The front lobby pair created two bulky/bright elements immediately at
    // the entry. Remove them; the rear pair keeps the lobby structurally framed.
    drawColumn(-8.0f,25.0f); drawColumn(8.0f,25.0f);
    drawChandelier(0,29.5f,1.15f);
    drawTexturedBox(-15.2f,0.85f,30.2f,7.2f,1.7f,1.6f,texWood,34);
    drawBox(-15.2f,1.88f,30.88f,6.2f,0.36f,0.18f,0.12f,0.10f,0.08f,55,0.6f);
    drawRoomSign("INFORMATION",-15.2f,3.4f,31.02f,180,4.4f);
    // Real museums do use visitor seating, but it should not block circulation.
    // Step 7: keep a single bench almost flush with the side wall.
// Plants are deliberately kept away from the seat so no leaves intersect it.
    drawPlant(-20.5f,32.5f,0.9f); drawPlant(11.2f,33.0f,0.72f);

    // ============================================================
    // FIRST FLOOR: ROAD TO INDEPENDENCE / 1971 WAR FOUNDATIONS
    // No freestanding board or large object sits on the entrance axis.
    // The side galleries carry the larger historical displays and memorials.
    // ============================================================

    // Clean wall-mounted entrance title; the old board that blocked the doorway is gone.
    drawRoomSign("ROAD TO INDEPENDENCE • 1971",0.0f,7.55f,35.72f,180,9.6f);

    // Six large wall photographs, all outside the immediate doorway zone.
    drawWallArtwork(-23.64f,5.55f,25.5f,4.35f,3.20f,texWarPhoto1,90,
                    "MASS MOVEMENT","PEOPLE & POLITICAL AWARENESS",true);
    drawWallArtwork(-23.64f,5.55f,2.0f,4.35f,3.20f,texWarPhoto2,90,
                    "LANGUAGE MOVEMENT","ROAD TO INDEPENDENCE",true);
    drawWallArtwork(-23.64f,5.55f,-22.5f,4.35f,3.20f,texWarPhoto4,90,
                    "DISPLACED COMMUNITIES","CIVILIAN EXPERIENCE",true);
    drawWallArtwork(23.64f,5.55f,25.5f,4.35f,3.20f,texWarPhoto5,-90,
                    "RESISTANCE & PEOPLE","EARLY 1971",true);
    drawWallArtwork(23.64f,5.55f,2.0f,4.35f,3.20f,texWarPhoto6,-90,
                    "DOCUMENTS & RADIO","COMMUNICATION & ORGANIZATION",true);
    drawWallArtwork(23.64f,5.55f,-22.5f,4.35f,3.20f,texWarPhoto8,-90,
                    "MARCH 1971","PUBLIC MOBILIZATION",true);


    // Additional wall artifacts use the open gaps between photographs.
    // This makes the side galleries feel curated rather than empty.
    drawWallWeaponRack(-23.60f,3.45f,14.2f, 90.0f,0.92f);
    drawArtworkLabel("HISTORIC ARMS DISPLAY",-23.12f,1.86f,14.2f,90);
    drawWallWeaponRack( 23.60f,3.45f,14.2f,-90.0f,0.92f);
    drawArtworkLabel("FIELD EQUIPMENT DISPLAY",23.12f,1.86f,14.2f,-90);
    drawWallWeaponRack(-23.60f,3.45f,-10.2f, 90.0f,0.92f);
    drawArtworkLabel("RESISTANCE ARTIFACTS",-23.12f,1.86f,-10.2f,90);
    drawWallMedalBoard( 23.60f,3.45f,-10.2f,-90.0f,0.92f);
    drawArtworkLabel("MEDALS & INSIGNIA",23.12f,1.86f,-10.2f,-90);

    // Two compact rear-wall collections use otherwise blank wall area.
    drawWallMedalBoard(-7.2f,4.15f,-35.60f,0.0f,0.88f);
    drawWallWeaponRack( 7.2f,4.15f,-35.60f,0.0f,0.88f);

    // Larger war-history displays. Scaling is applied only to the object so labels remain readable.
    const float EX = 1.30f;
    glPushMatrix(); glScalef(EX,EX,EX); drawPrintingPressDisplay(-15.0f,22.5f); glPopMatrix();
    drawArtworkLabel("NEWSPAPER & PRINTING",-15.0f,1.05f,25.40f,180);
    // Removed the duplicate FIELD MEDICAL UNIT display. The separate
    // MEDICAL KIT artifact case remains as the single medical equipment exhibit.
    glPushMatrix(); glScalef(EX,EX,EX); drawRefugeeReliefDisplay(-15.0f,-22.0f); glPopMatrix();
    drawArtworkLabel("CIVILIAN RELIEF & DISPLACEMENT",-15.0f,1.05f,-20.45f,180);

    // War Document Archive: moved forward on +Z so it is clear of the stair,
    // and rotated 180 degrees as one complete exhibit assembly.
    glPushMatrix();
        glScalef(EX,EX,EX);
        drawDocumentArchiveTable(-15.0f,-5.50f,180.0f);
    glPopMatrix();
    drawArtworkLabel("WAR DOCUMENT ARCHIVE",-15.0f,1.05f,-4.20f,180);
    glPushMatrix(); glScalef(EX,EX,EX); drawResistanceCampDisplay(15.0f,0.0f); glPopMatrix();
    drawArtworkLabel("FIELD CAMP & EQUIPMENT",15.0f,1.05f,1.25f,180);
    glPushMatrix(); glScalef(EX,EX,EX); drawVictoryArchiveDisplay(15.0f,-22.0f); glPopMatrix();
    drawArtworkLabel("VICTORY ARCHIVE",15.0f,1.05f,-20.45f,180);

    // Only one field-radio station is kept. The repeated radio display is replaced by a weapons case.
    glPushMatrix(); glScalef(EX,EX,EX); drawRadioDeskDisplay(-10.2f,11.0f); glPopMatrix();
    drawArtworkLabel("FIELD RADIO STATION",-10.2f,1.05f,12.15f,180);
    glPushMatrix(); glScalef(EX,EX,EX); drawFieldWeaponsDisplay(10.2f,11.0f); glPopMatrix();
    drawArtworkLabel("WEAPONS & FIELD EQUIPMENT",10.2f,1.05f,12.35f,180);


    // Additional room-scale dioramas fill the former empty corners while
    // preserving a clear central corridor.
    drawMortarAndCrateDisplay(-19.1f,17.1f,0.72f,18.0f);
    drawArtworkLabel("FIELD MORTAR & SUPPLY CRATES",-19.1f,0.70f,19.15f,180);
    drawFieldCommandPostDiorama(19.0f,-4.0f,0.72f,-15.0f);
    drawArtworkLabel("FIELD COMMAND POST",19.0f,0.70f,-1.65f,180);

    // Enlarged memorial zone. The four primary memorials now use the actual
    // transformation matrix so translation / rotation / scaling affect the
    // complete statue assembly instead of a separate floor gizmo.
    drawTransformedMemorial(0,drawSevenMarchMemorial,-8.65f,22.0f,1.10f);
    drawTransformedMemorial(1,drawFreedomFighterGroupMemorial,8.65f,22.0f,1.06f);
    // The Shaheed Minar has been moved outside and enlarged into a dedicated
    // memorial court. Its former indoor position is now an archival display.
    drawGlassCase(-8.65f,1.15f,7,180.0f);
    drawArtworkLabel("LANGUAGE MOVEMENT ARCHIVE",-8.65f,0.72f,2.72f,180);

    // Dedicated 1971 field medical unit remains as a separate themed exhibit.
    drawFieldMedicalUnitDisplay(-12.85f,1.10f,0.0f);

    // War-time medical volunteer memorial: rotate the complete statue and
    // its plaque 180 degrees so it faces the same exhibition direction.
    drawFieldNurseStatue(8.65f,1.15f,1.06f,180.0f);
    drawTransformedMemorial(2,drawMotherChildMemorial,-8.65f,-20.4f,1.08f);
    drawRadioOperatorMemorial(8.65f,-20.4f,1.06f);
    drawTransformedMemorial(3,drawVictoryFlagMemorial,0.0f,-31.0f,1.12f);

    // A denser but still walkable visitor population makes the museum feel occupied.
    drawMuseumVisitor(-18.3f,28.5f,0.88f,155);
    drawMuseumVisitor(18.3f,28.5f,0.86f,-155);
    drawMuseumVisitor(-18.2f,-28.8f,0.84f,45);
    drawMuseumVisitor(18.2f,-28.8f,0.82f,-45);
    drawMuseumVisitor(-3.6f,28.0f,0.90f,175);
    drawMuseumVisitor( 3.8f,16.5f,0.85f,-165);
    drawMuseumVisitor(-3.5f,-5.5f,0.84f,20);
    drawMuseumVisitor( 3.7f,-16.5f,0.88f,-25);
    drawMuseumVisitor(-17.2f,15.5f,0.80f,95);
    drawMuseumVisitor( 17.0f,-14.8f,0.82f,-92);
    drawMuseumVisitor(-10.8f,6.0f,0.82f,125);
    drawMuseumVisitor( 11.2f,5.0f,0.84f,-120);
    drawMuseumVisitor(-18.8f,-4.5f,0.80f,80);
    drawMuseumVisitor( 18.5f,18.5f,0.82f,-110);
    drawMuseumVisitor(-11.0f,-27.0f,0.83f,35);
    drawMuseumVisitor( 11.4f,-25.5f,0.84f,-40);

    drawRoomSign("PEOPLE & MOVEMENT",-6.0f,7.70f,10.5f,90,6.2f);
    drawRoomSign("RESISTANCE & SUPPORT",6.0f,7.70f,10.5f,-90,6.9f);
    drawRoomSign("CIVILIAN MEMORY",-6.0f,7.70f,-10.0f,90,5.8f);
    drawRoomSign("WAR ARCHIVE",6.0f,7.70f,-10.0f,-90,5.6f);

    // No decorative plants, bench, or generic mannequin on this floor: every floor object is theme-related.
    drawWallLamp(-5.5f,7.45f,29.0f,90);
    drawWallLamp( 5.5f,7.45f,29.0f,-90);
    drawWallLamp(-5.5f,7.30f,-23.0f,90);
    drawWallLamp( 5.5f,7.30f,-23.0f,-90);

    drawCeilingFan(0,16.0f,0.92f);
    drawCeilingFan(0,-2.0f,0.92f);
    drawCeilingFan(0,-20.0f,0.92f);
}


// Step 7 visual polish: low, wall-hugging guide lights.  These enrich the
// corridor without occupying visitor floor space or intersecting exhibits.
void drawLowGuideLights() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(float z : {22.0f, 8.0f, -9.0f, -22.0f}) {
        for(float x : {-5.66f, 5.66f}) {
            glColor4f(1.0f,0.72f,0.28f,0.70f);
            glPushMatrix();
            glTranslatef(x,0.42f,z);
            glScalef(0.05f,0.16f,0.72f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }
    }
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ================================================================
// Display cases and Liberation War artifact objects
// ================================================================

void drawBirdSculpture(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(scale,scale,scale);
    drawTexturedBox(0,0.42f,0,1.55f,0.84f,1.55f,texWood,32);
    drawScaledSphere(0,1.45f,0,0.72f,0.44f,0.38f,0.18f,0.43f,0.31f,55,0.48f);
    drawScaledSphere(0.58f,1.72f,0,0.30f,0.30f,0.28f,0.16f,0.39f,0.28f,55,0.48f);
    glPushMatrix(); glTranslatef(0.88f,1.72f,0); glRotatef(-90,0,0,1); drawConeY(0,0,0,0.12f,0.36f,0.78f,0.52f,0.10f); glPopMatrix();
    // Wings and tail
    drawScaledSphere(-0.05f,1.52f,0.34f,0.52f,0.14f,0.28f,0.10f,0.31f,0.23f,45,0.35f);
    drawScaledSphere(-0.05f,1.52f,-0.34f,0.52f,0.14f,0.28f,0.10f,0.31f,0.23f,45,0.35f);
    drawBox(-0.72f,1.45f,0,0.62f,0.12f,0.28f,0.12f,0.30f,0.22f,40,0.30f);
    glPopMatrix();
}


void drawWarRifleArtifact() {
    // Generic period rifle silhouette for a historical display case.
    drawBox(-0.55f,1.10f,0,1.20f,0.16f,0.18f,0.29f,0.16f,0.08f,28,0.18f);
    drawCylinderBetween(Vec3(-0.05f,1.13f,0),Vec3(1.18f,1.13f,0),0.045f,0.20f,0.21f,0.20f);
    drawCylinderBetween(Vec3(0.36f,1.14f,0),Vec3(0.18f,0.72f,0),0.075f,0.17f,0.09f,0.045f);
    drawBox(1.25f,1.13f,0,0.28f,0.06f,0.08f,0.24f,0.23f,0.20f,35,0.35f);
    drawCylinderBetween(Vec3(0.96f,1.18f,0),Vec3(1.42f,1.18f,0),0.018f,0.46f,0.31f,0.10f);
}

void drawWarPistolArtifact() {
    drawBox(-0.32f,1.02f,0,0.78f,0.18f,0.22f,0.19f,0.18f,0.16f,45,0.45f);
    drawCylinderBetween(Vec3(0.25f,1.04f,0),Vec3(0.85f,1.04f,0),0.055f,0.21f,0.21f,0.20f);
    drawCylinderBetween(Vec3(-0.02f,0.99f,0),Vec3(-0.18f,0.63f,0),0.075f,0.16f,0.09f,0.04f);
    drawSphere(0.46f,1.12f,0,0.07f,0.55f,0.42f,0.18f,35,0.52f);
}

void drawFieldRadioArtifact() {
    drawBox(0,1.28f,0,1.55f,1.05f,0.92f,0.16f,0.18f,0.17f,32,0.30f);
    drawBox(0,1.74f,0.49f,1.10f,0.30f,0.06f,0.12f,0.14f,0.13f,35,0.22f);
    for(int i=0;i<3;i++) drawCylinderY(-0.47f+i*0.46f,1.80f,0.55f,0.065f,0.08f,0.55f,0.42f,0.15f,30,0.45f);
    drawCylinderBetween(Vec3(0.45f,1.76f,0),Vec3(0.78f,2.35f,0.05f),0.022f,0.18f,0.18f,0.16f);
    drawSphere(-0.52f,1.32f,0.49f,0.075f,0.72f,0.49f,0.14f,30,0.52f);
}

void drawWarHelmetArtifact() {
    drawScaledSphere(0,1.25f,0,0.74f,0.36f,0.62f,0.19f,0.25f,0.20f,28,0.35f);
    drawBox(0,1.05f,0.18f,1.02f,0.10f,0.55f,0.17f,0.22f,0.18f,25,0.26f);
    drawCylinderBetween(Vec3(-0.48f,1.10f,0.0f),Vec3(0.48f,1.10f,0.0f),0.018f,0.12f,0.14f,0.11f);
}

void drawWarDocumentArtifact() {
    // Open field notebook / operational document, mounted flat for safe viewing.
    drawBox(0,1.05f,0,1.55f,0.10f,1.12f,0.76f,0.70f,0.55f,18,0.18f);
    for(int i=0;i<6;i++)
        drawBox(-0.46f,1.11f, -0.40f+i*0.15f, 0.80f,0.012f,0.018f,0.18f,0.17f,0.14f,8,0.05f);
    drawBox(0,1.12f,0,0.06f,0.01f,0.98f,0.50f,0.16f,0.08f,10,0.08f);
}

void drawWarMedicalKitArtifact() {
    drawBox(0,1.12f,0,1.40f,0.62f,1.00f,0.47f,0.29f,0.16f,24,0.18f);
    drawBox(0,1.46f,0,1.05f,0.06f,0.72f,0.56f,0.46f,0.28f,25,0.24f);
    drawBox(0,1.47f,0,0.16f,0.07f,0.72f,0.80f,0.78f,0.70f,12,0.18f);
    drawBox(0,1.47f,0,1.05f,0.07f,0.16f,0.80f,0.78f,0.70f,12,0.18f);
}

void drawWarRucksackArtifact() {
    drawScaledSphere(0,1.25f,0,0.78f,0.95f,0.50f,0.28f,0.30f,0.24f,16,0.18f);
    drawBox(0,1.70f,-0.02f,0.36f,0.42f,0.18f,0.23f,0.25f,0.20f,20,0.18f);
    drawCylinderBetween(Vec3(-0.50f,1.40f,0),Vec3(-0.62f,2.03f,0),0.055f,0.18f,0.19f,0.15f);
    drawCylinderBetween(Vec3( 0.50f,1.40f,0),Vec3( 0.62f,2.03f,0),0.055f,0.18f,0.19f,0.15f);
    drawBox(0,1.25f,0.53f,0.72f,0.10f,0.06f,0.56f,0.44f,0.20f,20,0.15f);
}

void drawWarBinocularArtifact() {
    glPushMatrix();
    glTranslatef(0,1.20f,0);
    glRotatef(-8,0,0,1);
    drawCylinderBetween(Vec3(-0.33f,0,0),Vec3(-0.33f,0.45f,0),0.13f,0.16f,0.15f,0.13f);
    drawCylinderBetween(Vec3(0.33f,0,0),Vec3(0.33f,0.45f,0),0.13f,0.16f,0.15f,0.13f);
    drawCylinderBetween(Vec3(-0.33f,0.42f,0),Vec3(0.33f,0.42f,0),0.075f,0.22f,0.21f,0.18f);
    drawSphere(-0.33f,0.05f,0,0.18f,0.10f,0.07f,0.05f,16,0.12f);
    drawSphere(0.33f,0.05f,0,0.18f,0.10f,0.07f,0.05f,16,0.12f);
    glPopMatrix();
}

void drawGlassCase(float x,float z,int artifactType=0,float rotY=0) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);

    const float W=3.0f, D=2.4f, H=2.65f;
    const float baseY=0.35f, glassBottom=0.72f;

    // Low museum plinth -- intentionally much smaller than the old white block.
    drawTexturedBox(0,baseY,0,W,0.70f,D,texWood,30);
    drawBox(0,0.73f,0,W+0.10f,0.08f,D+0.10f,0.58f,0.42f,0.15f,58,0.60f);

    // Every case is now a Liberation War artifact; no generic art objects remain.
    if(artifactType==3) drawWarRifleArtifact();
    else if(artifactType==4) drawFieldRadioArtifact();
    else if(artifactType==5) drawWarHelmetArtifact();
    else if(artifactType==6) drawWarPistolArtifact();
    else if(artifactType==7) drawWarDocumentArtifact();
    else if(artifactType==8) drawWarMedicalKitArtifact();
    else if(artifactType==9) drawWarRucksackArtifact();
    else if(artifactType==10) drawWarBinocularArtifact();
    else drawWarRifleArtifact();

    // Thin dark/brass frame: four corner posts + top frame.
    const float px=W*0.5f-0.08f, pz=D*0.5f-0.08f;
    for(float xx: {-px,px}) for(float zz: {-pz,pz})
        drawBox(xx,glassBottom+H*0.5f,zz,0.07f,H,0.07f,0.18f,0.16f,0.13f,70,0.72f);
    drawBox(0,glassBottom+H,D*0.0f,W,0.07f,0.07f,0.38f,0.28f,0.10f,62,0.65f);
    drawBox(0,glassBottom+H, pz,W,0.07f,0.07f,0.38f,0.28f,0.10f,62,0.65f);
    drawBox(0,glassBottom+H,-pz,W,0.07f,0.07f,0.38f,0.28f,0.10f,62,0.65f);
    drawBox( px,glassBottom+H,0,0.07f,0.07f,D,0.38f,0.28f,0.10f,62,0.65f);
    drawBox(-px,glassBottom+H,0,0.07f,0.07f,D,0.38f,0.28f,0.10f,62,0.65f);

    // IMPORTANT FIX: do not draw filled alpha cubes for the glass panes.
    // On some FreeGLUT/fixed-pipeline systems those overlapping faces render as
    // opaque white boxes. Instead, use subtle transparent edge highlights.
    // The artifact remains fully visible while the brass frame still reads as
    // a protective museum display case.
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.72f,0.90f,1.0f,0.34f);
    glLineWidth(1.6f);

    const float y0=glassBottom+0.04f, y1=glassBottom+H-0.04f;
    const float ex=px-0.01f, ez=pz-0.01f;
    glBegin(GL_LINES);
    // vertical glass edge highlights
    glVertex3f(-ex,y0,-ez); glVertex3f(-ex,y1,-ez);
    glVertex3f( ex,y0,-ez); glVertex3f( ex,y1,-ez);
    glVertex3f(-ex,y0, ez); glVertex3f(-ex,y1, ez);
    glVertex3f( ex,y0, ez); glVertex3f( ex,y1, ez);
    // lower and upper perimeter highlights
    glVertex3f(-ex,y0,-ez); glVertex3f( ex,y0,-ez);
    glVertex3f( ex,y0,-ez); glVertex3f( ex,y0, ez);
    glVertex3f( ex,y0, ez); glVertex3f(-ex,y0, ez);
    glVertex3f(-ex,y0, ez); glVertex3f(-ex,y0,-ez);
    glVertex3f(-ex,y1,-ez); glVertex3f( ex,y1,-ez);
    glVertex3f( ex,y1,-ez); glVertex3f( ex,y1, ez);
    glVertex3f( ex,y1, ez); glVertex3f(-ex,y1, ez);
    glVertex3f(-ex,y1, ez); glVertex3f(-ex,y1,-ez);
    glEnd();
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawAdditionalGalleryObjects() {
    // Extra compact artifact cases occupy previously sparse side-room zones.
    // Their positions stay close to the walls so the main corridor remains open.
    drawGlassCase(-20.0f, 14.5f,5,90);
    drawArtworkLabel("FIELD HELMET",-18.35f,0.62f,14.5f,90);

    drawGlassCase( 20.0f, 14.5f,10,-90);
    drawArtworkLabel("OBSERVATION BINOCULARS",18.35f,0.62f,14.5f,-90);

    drawGlassCase(-20.0f,-14.5f,7,90);
    drawArtworkLabel("WARTIME DOCUMENTS",-18.35f,0.62f,-14.5f,90);

    drawGlassCase( 20.0f,-14.5f,9,-90);
    drawArtworkLabel("FREEDOM FIGHTER FIELD PACK",18.35f,0.62f,-14.5f,-90);
}


// ================================================================
// Liberation War themed people / memorial elements
// ================================================================

void drawMuseumVisitor(float x,float z,float scale=1.0f,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);
    glScalef(scale,scale,scale);
    drawCylinderBetween(Vec3(-0.18f,0.0f,0),Vec3(-0.20f,0.78f,0),0.09f,0.09f,0.10f,0.12f);
    drawCylinderBetween(Vec3( 0.18f,0.0f,0),Vec3( 0.20f,0.78f,0),0.09f,0.09f,0.10f,0.12f);
    // Deterministic clothing variation prevents a larger crowd from looking cloned.
    float vr=0.16f, vg=0.27f, vb=0.46f;
    int variant=(int)std::fabs(x*0.73f+z*0.31f)%4;
    if(variant==1){ vr=0.18f; vg=0.42f; vb=0.26f; }
    else if(variant==2){ vr=0.48f; vg=0.22f; vb=0.18f; }
    else if(variant==3){ vr=0.36f; vg=0.27f; vb=0.52f; }
    drawScaledSphere(0,1.25f,0,0.43f,0.60f,0.27f,vr,vg,vb,16,0.18f);
    drawSphere(0,2.03f,0,0.25f,0.52f,0.35f,0.24f,16,0.14f);
    drawCylinderBetween(Vec3(-0.34f,1.52f,0),Vec3(-0.58f,0.82f,0),0.065f,vr,vg,vb);
    drawCylinderBetween(Vec3( 0.34f,1.52f,0),Vec3( 0.58f,0.82f,0),0.065f,vr,vg,vb);
    glPopMatrix();
}

void drawFreedomFighterMannequin(float x,float z,float scale=1.0f,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);
    glScalef(scale,scale,scale);
    drawCylinderBetween(Vec3(-0.18f,0.02f,0),Vec3(-0.20f,0.95f,0),0.10f,0.07f,0.12f,0.09f);
    drawCylinderBetween(Vec3( 0.18f,0.02f,0),Vec3( 0.20f,0.95f,0),0.10f,0.07f,0.12f,0.09f);
    drawScaledSphere(0,1.48f,0,0.47f,0.66f,0.30f,0.27f,0.31f,0.20f,18,0.20f);
    drawBox(0,1.63f,0.27f,0.16f,0.28f,0.05f,0.55f,0.48f,0.28f,16,0.10f);
    drawSphere(0,2.35f,0,0.26f,0.48f,0.39f,0.30f,16,0.12f);
    drawScaledSphere(0,2.58f,0,0.38f,0.14f,0.31f,0.22f,0.24f,0.18f,16,0.18f);
    drawCylinderBetween(Vec3(-0.34f,1.72f,0),Vec3(-0.63f,1.10f,0),0.07f,0.25f,0.30f,0.18f);
    drawCylinderBetween(Vec3( 0.34f,1.72f,0),Vec3( 0.63f,1.10f,0),0.07f,0.25f,0.30f,0.18f);
    drawCylinderBetween(Vec3(-0.48f,1.10f,0.16f),Vec3(0.55f,2.18f,0.16f),0.035f,0.19f,0.17f,0.12f);
    drawBox(0.48f,2.15f,0.16f,0.18f,0.07f,0.07f,0.18f,0.16f,0.10f,25,0.18f);
    glPopMatrix();
}

void drawBangladeshFlag(float x,float z,float poleH=5.2f,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(scale,scale,scale);
    drawBox(0,poleH*0.5f,0,0.12f,poleH,0.12f,0.17f,0.16f,0.15f,28,0.30f);
    glDisable(GL_LIGHTING);
    glColor3f(0.02f,0.34f,0.20f);
    glBegin(GL_QUADS);
    glVertex3f(0.06f,poleH-0.65f,0.0f); glVertex3f(2.65f,poleH-0.65f,0.0f);
    glVertex3f(2.65f,poleH-1.95f,0.0f); glVertex3f(0.06f,poleH-1.95f,0.0f);
    glEnd();
    glColor3f(0.78f,0.07f,0.07f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(1.37f,poleH-1.30f,0.012f);
    for(int i=0;i<=28;i++){
        float a=2.0f*PI*float(i)/28.0f;
        glVertex3f(1.37f+0.43f*std::cos(a),poleH-1.30f+0.43f*std::sin(a),0.012f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// ================================================================
// Ground-floor Liberation War memorial sculptures + unique displays
// These replace the old animal artwork locations and repetitive generic cases.
// ================================================================

void drawStatueBase(float x,float z,float w,float d,const std::string& label,
                    float plaqueY=0.98f,float plaqueLift=0.16f,
                    float plaqueTextScale=0.00066f,float plaqueWidthScale=1.0f) {
    // Compact memorial plinth. The name plate is deliberately mounted on the
    // FRONT (+Z) face because visitors approach the sculptures from the entry.
    drawTexturedBox(x,0.48f,z,w,0.96f,d,texFloor,42);
    drawBox(x,1.02f,z, w+0.16f,0.10f,d+0.16f,0.34f,0.28f,0.20f,55,0.42f);

    const float plaqueW=clampf((1.70f + 0.050f*float(label.size()))*plaqueWidthScale,2.05f,3.25f);
    const float plaqueD=0.075f;
    const float plaqueZ=z + d*0.50f + plaqueLift;

    // Small front-mounted plate with two short supports.
    drawBox(x,plaqueY,plaqueZ,plaqueW,0.30f,plaqueD,0.22f,0.16f,0.09f,28,0.28f);
    drawBox(x,plaqueY,plaqueZ+0.040f,plaqueW-0.08f,0.22f,0.018f,0.86f,0.82f,0.70f,12,0.06f);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.07f,0.065f,0.055f);
    glPushMatrix();
    // IMPORTANT: +Z is the visitor-facing side of the plaque.
    glTranslatef(x-plaqueW*0.43f,plaqueY-0.03f,plaqueZ+0.052f);
    glScalef(plaqueTextScale,plaqueTextScale,plaqueTextScale);
    for(char c:label) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void drawSevenMarchMemorial(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,1.02f,z);
    glScalef(scale,scale,scale);
    setMaterial(0.16f,0.13f,0.10f,82,0.72f);
    // podium
    drawBox(0,0.75f,0,1.35f,1.50f,0.82f,0.20f,0.17f,0.13f,48,0.50f);
    drawBox(0,1.55f,0.04f,1.72f,0.12f,0.95f,0.24f,0.20f,0.15f,48,0.50f);
    // human figure
    drawCylinderBetween(Vec3(0,1.62f,0),Vec3(0,3.00f,0),0.22f,0.18f,0.15f,0.11f);
    drawSphere(0,3.35f,0,0.25f,0.18f,0.15f,0.11f,62,0.55f);
    // raised right arm / open-hand gesture
    drawCylinderBetween(Vec3(0.20f,2.62f,0),Vec3(0.55f,3.00f,0),0.075f,0.18f,0.15f,0.11f);
    drawCylinderBetween(Vec3(0.55f,3.00f,0),Vec3(0.66f,3.55f,0),0.065f,0.18f,0.15f,0.11f);
    drawScaledSphere(0.66f,3.72f,0,0.11f,0.17f,0.11f,0.18f,0.15f,0.11f,34,0.55f);
    // left arm toward podium
    drawCylinderBetween(Vec3(-0.20f,2.63f,0),Vec3(-0.47f,2.05f,0.04f),0.070f,0.18f,0.15f,0.11f);
    // lapel / shirt folds
    drawBox(0,2.43f,0.20f,0.08f,0.72f,0.03f,0.28f,0.23f,0.17f,28,0.32f);
    // microphone cluster
    drawCylinderBetween(Vec3(0,1.67f,0.22f),Vec3(0,2.05f,0.34f),0.018f,0.10f,0.10f,0.09f);
    drawSphere(0,2.10f,0.34f,0.055f,0.08f,0.07f,0.06f,30,0.35f);
    glPopMatrix();
    // Lower, smaller plaque so the memorial sculpture remains dominant and
    // the label does not float into the visitor sightline.
    drawStatueBase(x,z,3.05f,2.55f,"7 MARCH SPEECH MEMORIAL",
                   0.98f,0.26f,0.00064f,0.90f);
}

void drawFreedomFighterStatue(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,1.05f,z);
    glScalef(scale,scale,scale);
    setMaterial(0.19f,0.17f,0.13f,78,0.68f);
    // boots / legs
    drawCylinderBetween(Vec3(-0.20f,0.15f,0),Vec3(-0.26f,1.18f,0),0.105f,0.20f,0.18f,0.14f);
    drawCylinderBetween(Vec3( 0.20f,0.15f,0),Vec3( 0.24f,1.18f,0),0.105f,0.20f,0.18f,0.14f);
    drawBox(-0.26f,0.10f,0.06f,0.48f,0.18f,0.62f,0.12f,0.10f,0.08f,38,0.48f);
    drawBox( 0.24f,0.10f,0.06f,0.48f,0.18f,0.62f,0.12f,0.10f,0.08f,38,0.48f);
    // torso and vest
    drawScaledSphere(0,1.58f,0,0.56f,0.75f,0.34f,0.19f,0.17f,0.13f,62,0.55f);
    drawBox(0,1.64f,0.30f,0.34f,0.52f,0.05f,0.25f,0.22f,0.17f,35,0.40f);
    // head + cap
    drawSphere(0,2.48f,0,0.29f,0.20f,0.17f,0.13f,62,0.48f);
    drawScaledSphere(0,2.69f,0,0.38f,0.12f,0.34f,0.12f,0.10f,0.08f,44,0.50f);
    // arms holding the rifle
    drawCylinderBetween(Vec3(-0.40f,1.78f,0),Vec3(-0.70f,1.23f,0.10f),0.075f,0.19f,0.17f,0.13f);
    drawCylinderBetween(Vec3( 0.40f,1.78f,0),Vec3( 0.70f,1.35f,0.10f),0.075f,0.19f,0.17f,0.13f);
    // museum-safe stylized rifle on sling pose
    drawCylinderBetween(Vec3(-0.48f,1.30f,0.19f),Vec3(0.54f,2.08f,0.19f),0.030f,0.10f,0.09f,0.07f);
    drawBox(0.48f,2.12f,0.19f,0.16f,0.06f,0.06f,0.10f,0.09f,0.07f,30,0.35f);
    // scarf / belt details
    drawBox(0,1.78f,0.30f,0.70f,0.11f,0.05f,0.11f,0.10f,0.08f,25,0.40f);
    glPopMatrix();
    drawStatueBase(x,z,2.85f,2.55f,"FREEDOM FIGHTER MEMORIAL");
}

void drawMotherChildMemorial(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,1.02f,z);
    glScalef(scale,scale,scale);
    setMaterial(0.26f,0.22f,0.18f,74,0.62f);
    // kneeling mother
    drawScaledSphere(-0.18f,1.30f,0,0.64f,0.55f,0.38f,0.27f,0.23f,0.19f,58,0.48f);
    drawSphere(-0.28f,2.02f,0,0.27f,0.27f,0.23f,0.19f,58,0.46f);
    // child held at chest
    drawScaledSphere(0.28f,1.63f,0.02f,0.32f,0.48f,0.24f,0.30f,0.26f,0.21f,52,0.42f);
    drawSphere(0.30f,2.13f,0.02f,0.18f,0.30f,0.26f,0.21f,52,0.40f);
    // arms wrapping around child
    drawCylinderBetween(Vec3(-0.54f,1.58f,0),Vec3(0.18f,1.94f,0.18f),0.075f,0.25f,0.22f,0.18f);
    drawCylinderBetween(Vec3(-0.38f,1.78f,0),Vec3(0.44f,1.54f,-0.12f),0.070f,0.25f,0.22f,0.18f);
    // shawl fold
    drawScaledSphere(-0.40f,1.62f,0.16f,0.48f,0.62f,0.08f,0.18f,0.16f,0.13f,38,0.35f);
    glPopMatrix();
    drawStatueBase(x,z,3.05f,2.55f,"CIVILIAN MEMORY MEMORIAL");
}

void drawVictoryFlagMemorial(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,1.02f,z);
    glScalef(scale,scale,scale);
    // central stone/bronze shard
    drawBox(0,1.20f,0,1.15f,2.40f,0.75f,0.20f,0.18f,0.14f,68,0.62f);
    // flag pole and simplified raised flag
    drawCylinderBetween(Vec3(0,1.55f,0.12f),Vec3(0,4.55f,0.12f),0.035f,0.14f,0.12f,0.10f);
    glDisable(GL_LIGHTING);
    glColor3f(0.02f,0.38f,0.22f);
    glBegin(GL_QUADS);
    glVertex3f(0.03f,4.42f,0.12f); glVertex3f(1.52f,4.42f,0.12f);
    glVertex3f(1.52f,3.70f,0.12f); glVertex3f(0.03f,3.70f,0.12f);
    glEnd();
    glColor3f(0.80f,0.07f,0.06f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.78f,4.06f,0.135f);
    for(int i=0;i<=24;i++){
        float a=2.0f*PI*float(i)/24.0f;
        glVertex3f(0.78f+0.24f*std::cos(a),4.06f+0.24f*std::sin(a),0.135f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    // abstracted relief figures, facing the flag
    drawScaledSphere(-0.42f,2.85f,0,0.24f,0.58f,0.20f,0.21f,0.18f,0.14f,55,0.45f);
    drawSphere(-0.42f,3.53f,0,0.21f,0.21f,0.18f,0.14f,55,0.45f);
    drawCylinderBetween(Vec3(-0.50f,3.18f,0),Vec3(-0.13f,3.72f,0.08f),0.06f,0.21f,0.18f,0.13f);
    glPopMatrix();
    drawStatueBase(x,z,3.05f,2.55f,"VICTORY & INDEPENDENCE MEMORIAL");
}


void drawFreedomFighterGroupMemorial(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,1.02f,z);
    glScalef(scale,scale,scale);
    setMaterial(0.18f,0.16f,0.13f,78,0.66f);
    drawBox(0,0.62f,0,3.55f,1.24f,2.10f,0.20f,0.17f,0.13f,50,0.50f);
    drawBox(0,1.30f,0.06f,3.12f,0.10f,1.72f,0.24f,0.20f,0.15f,45,0.45f);
    const float pxs[3]={-0.92f,0.0f,0.92f};
    const float pys[3]={1.46f,1.60f,1.50f};
    for(int i=0;i<3;i++){
        float px=pxs[i], py=pys[i];
        drawCylinderBetween(Vec3(px-0.08f,1.38f,0),Vec3(px-0.10f,py,0),0.10f,0.18f,0.15f,0.12f);
        drawCylinderBetween(Vec3(px+0.08f,1.38f,0),Vec3(px+0.10f,py,0.02f),0.10f,0.18f,0.15f,0.12f);
        drawScaledSphere(px,py+0.42f,0,0.28f,0.45f,0.22f,0.19f,0.16f,0.12f,42,0.48f);
        drawSphere(px,py+0.95f,0,0.21f,0.17f,0.13f,0.10f,42,0.46f);
    }
    drawCylinderBetween(Vec3(-1.18f,1.80f,0),Vec3(-1.45f,2.30f,0.03f),0.065f,0.18f,0.15f,0.11f);
    drawCylinderBetween(Vec3(-0.62f,1.82f,0),Vec3(-0.30f,2.28f,0.02f),0.065f,0.18f,0.15f,0.11f);
    drawCylinderBetween(Vec3(0.82f,1.83f,0),Vec3(1.20f,2.18f,0.02f),0.065f,0.18f,0.15f,0.11f);
    drawCylinderBetween(Vec3(0.0f,1.35f,0.18f),Vec3(0.0f,4.18f,0.18f),0.034f,0.14f,0.12f,0.10f);
    glDisable(GL_LIGHTING);
    glColor3f(0.02f,0.34f,0.20f);
    glBegin(GL_QUADS);
    glVertex3f(0.04f,4.08f,0.18f); glVertex3f(1.35f,4.08f,0.18f);
    glVertex3f(1.35f,3.38f,0.18f); glVertex3f(0.04f,3.38f,0.18f);
    glEnd();
    glColor3f(0.78f,0.07f,0.07f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.70f,3.73f,0.20f);
    for(int i=0;i<=22;i++){ float a=2.0f*PI*i/22.0f; glVertex3f(0.70f+0.20f*std::cos(a),3.73f+0.20f*std::sin(a),0.20f); }
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
    drawStatueBase(x,z,3.70f,2.35f,"FREEDOM FIGHTER GROUP MEMORIAL",
                   0.98f,0.24f,0.00062f,0.88f);
}

void drawFieldNurseStatue(float x,float z,float scale,float rotY) {
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);
    glTranslatef(0.0f,1.02f,0.0f);
    glScalef(scale,scale,scale);
    setMaterial(0.28f,0.23f,0.19f,72,0.60f);
    drawBox(0,0.62f,0,2.95f,1.24f,2.25f,0.21f,0.18f,0.15f,48,0.46f);
    drawScaledSphere(0,1.72f,0,0.48f,0.78f,0.30f,0.72f,0.70f,0.64f,42,0.34f);
    drawScaledSphere(0,2.62f,0,0.25f,0.21f,0.18f,0.75f,0.68f,0.58f,42,0.38f);
    drawScaledSphere(0,2.78f,0,0.33f,0.10f,0.28f,0.72f,0.68f,0.60f,36,0.34f);
    drawCylinderBetween(Vec3(-0.38f,1.96f,0),Vec3(-0.72f,1.36f,0.10f),0.075f,0.75f,0.70f,0.64f);
    drawCylinderBetween(Vec3(0.38f,1.96f,0),Vec3(0.72f,1.38f,0.10f),0.075f,0.75f,0.70f,0.64f);
    drawBox(0.90f,1.12f,0.16f,0.56f,0.65f,0.45f,0.24f,0.18f,0.12f,20,0.20f);
    drawBox(0,1.84f,0.31f,0.09f,0.40f,0.03f,0.90f,0.86f,0.80f,12,0.16f);
    drawBox(0,1.84f,0.33f,0.34f,0.08f,0.03f,0.90f,0.86f,0.80f,12,0.16f);
    glPopMatrix();

    // Rotate the base + front plaque with the statue so the label stays
    // attached to the same visitor-facing side after the 180 degree turn.
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);
    drawStatueBase(0.0f,0.0f,3.15f,2.40f,"WAR-TIME MEDICAL VOLUNTEER");
    glPopMatrix();
}

void drawShaheedMinarMemorial(float x,float z,float scale,float rotY) {
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);
    glTranslatef(0.0f,1.02f,0.0f);
    glScalef(scale,scale,scale);
    drawBox(0,0.52f,0,3.20f,1.04f,2.20f,0.48f,0.45f,0.40f,44,0.35f);
    const float xs[5]={-0.86f,-0.43f,0.0f,0.43f,0.86f};
    const float hs[5]={2.30f,2.85f,3.45f,2.85f,2.30f};
    for(int i=0;i<5;i++) drawBox(xs[i],1.05f+hs[i]*0.50f,0,0.24f,hs[i],0.30f,0.73f,0.70f,0.64f,45,0.40f);
    drawScaledSphere(0,2.12f,0.19f,0.66f,0.66f,0.12f,0.78f,0.08f,0.07f,42,0.30f);
    glPopMatrix();

    // Keep the plinth + front plaque inside the same 180° transform so the
    // memorial and its visitor-facing label turn together.
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);
    drawStatueBase(0.0f,0.0f,3.35f,2.30f,"SHAHEED MINAR • 1952 → 1971");
    glPopMatrix();
}

void drawRadioOperatorMemorial(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,1.02f,z);
    glScalef(scale,scale,scale);
    setMaterial(0.19f,0.17f,0.13f,74,0.62f);
    drawBox(0,0.58f,0,3.00f,1.16f,2.15f,0.20f,0.17f,0.13f,46,0.45f);
    drawScaledSphere(0,1.66f,0,0.50f,0.68f,0.31f,0.19f,0.17f,0.13f,42,0.46f);
    drawSphere(0,2.55f,0,0.25f,0.21f,0.17f,0.13f,42,0.42f);
    drawScaledSphere(0,2.71f,0,0.34f,0.11f,0.30f,0.12f,0.10f,0.08f,34,0.46f);
    drawCylinderBetween(Vec3(-0.38f,1.92f,0),Vec3(-0.70f,1.48f,0.04f),0.075f,0.19f,0.17f,0.13f);
    drawCylinderBetween(Vec3(0.38f,1.92f,0),Vec3(0.60f,1.44f,0.05f),0.075f,0.19f,0.17f,0.13f);
    drawBox(0.55f,1.16f,0.18f,0.85f,0.66f,0.72f,0.15f,0.18f,0.16f,30,0.26f);
    drawCylinderBetween(Vec3(0.64f,1.48f,0.18f),Vec3(1.08f,2.30f,0.24f),0.022f,0.18f,0.18f,0.16f);
    glPopMatrix();
    drawStatueBase(x,z,3.15f,2.25f,"FIELD RADIO OPERATOR MEMORIAL");
}

// Applies the project's translation, rotation and scaling matrix to the full
// memorial assembly, including its pedestal and front-facing name plate.
void drawTransformedMemorial(int id, void (*drawMemorial)(float,float,float),
                             float x,float z,float baseScale) {
    const ObjectTransform& t=artXform[id];
    glPushMatrix();
    glTranslatef(x+t.tx,t.ty,z+t.tz);
    glRotatef(t.rotY,0,1,0);
    glScalef(t.scale,t.scale,t.scale);
    drawMemorial(0.0f,0.0f,baseScale);
    glPopMatrix();
}

void drawPrintingPressDisplay(float x,float z) {
    glPushMatrix(); glTranslatef(x,0,z);
    drawTexturedBox(0,0.55f,0,2.9f,1.1f,2.0f,texWood,34);
    drawBox(0,1.28f,0.58f,1.95f,0.10f,0.25f,0.18f,0.14f,0.09f,30,0.25f);
    drawCylinderY(0.55f,1.75f,0.54f,0.16f,0.68f,0.16f,0.13f,0.09f,30,0.35f);
    drawCylinderBetween(Vec3(-0.90f,1.76f,0.56f),Vec3(0.55f,1.76f,0.56f),0.035f,0.18f,0.15f,0.10f);
    drawBox(-0.75f,1.36f,0.60f,0.62f,0.05f,0.72f,0.70f,0.65f,0.50f,12,0.08f);
    drawBox(0.80f,1.40f,0.58f,0.50f,0.16f,0.74f,0.78f,0.69f,0.49f,12,0.08f);
    glPopMatrix();
    drawArtworkLabel("NEWSPAPER & PRINTING",x,0.78f,z+1.25f,180);
}

void drawRadioDeskDisplay(float x,float z) {
    glPushMatrix(); glTranslatef(x,0,z);
    drawBox(0,0.68f,0,2.8f,0.16f,1.45f,0.28f,0.20f,0.12f,30,0.35f);
    for(float sx: {-1.10f,1.10f}) for(float sz: {-0.46f,0.46f})
        drawBox(sx,0.30f,sz,0.14f,0.60f,0.14f,0.20f,0.16f,0.10f,28,0.30f);
    drawBox(0,1.35f,-0.05f,1.55f,0.95f,0.80f,0.16f,0.18f,0.17f,32,0.30f);
    drawBox(-0.30f,1.55f,0.39f,0.55f,0.28f,0.06f,0.10f,0.12f,0.11f,28,0.25f);
    drawCylinderBetween(Vec3(0.44f,1.55f,-0.02f),Vec3(0.62f,2.35f,0.04f),0.022f,0.18f,0.18f,0.16f);
    drawSphere(-0.52f,1.43f,0.42f,0.08f,0.72f,0.48f,0.13f,28,0.48f);
    glPopMatrix();
    drawArtworkLabel("FIELD RADIO STATION",x,0.78f,z+0.90f,180);
}

void drawRefugeeReliefDisplay(float x,float z) {
    glPushMatrix(); glTranslatef(x,0,z);
    drawTexturedBox(0,0.45f,0,2.7f,0.90f,1.75f,texWood,32);
    drawBox(-0.63f,1.18f,0,1.00f,0.44f,0.85f,0.35f,0.24f,0.12f,26,0.22f);
    drawBox(0.58f,1.12f,0,0.75f,0.14f,0.75f,0.62f,0.49f,0.27f,18,0.18f);
    drawScaledSphere(0.56f,1.40f,0,0.42f,0.15f,0.42f,0.74f,0.67f,0.47f,18,0.10f);
    drawBox(0.15f,1.18f,-0.42f,0.75f,0.08f,0.45f,0.55f,0.45f,0.32f,18,0.08f);
    glPopMatrix();
    drawArtworkLabel("CIVILIAN RELIEF & DISPLACEMENT",x,0.78f,z+1.05f,180);
}


void drawFieldMedicalUnitDisplay(float x,float z,float rotY) {
    // Compact treatment post inspired by a 1971 field medical station:
    // stretcher, canopy, medical chest and a clear red-cross marker.
    glPushMatrix();
    glTranslatef(x,0.0f,z);
    glRotatef(rotY,0,1,0);

    // raised timber platform
    drawTexturedBox(0,0.38f,0,3.15f,0.76f,2.55f,texWood,34);

    // stretcher
    drawBox(0,0.98f,0,2.15f,0.14f,0.82f,0.62f,0.56f,0.45f,22,0.16f);
    for(float sx : {-0.88f,0.88f}) {
        drawCylinderY(sx,0.52f,0,0.045f,0.46f,
                      0.24f,0.22f,0.19f,14,1);
        drawCylinderY(sx,0.52f,0.29f,0.045f,0.46f,
                      0.24f,0.22f,0.19f,14,1);
        drawCylinderY(sx,0.52f,-0.29f,0.045f,0.46f,
                      0.24f,0.22f,0.19f,14,1);
    }

    // canvas canopy
    for(float sx : {-1.20f,1.20f})
        for(float sz : {-0.88f,0.88f})
            drawCylinderY(sx,0.82f,sz,0.055f,2.30f,
                          0.22f,0.30f,0.22f,14,1);

    drawBox(0,3.16f,0,3.00f,0.16f,2.15f,
            0.33f,0.40f,0.30f,8.0f,0.06f);

    // medical supply chest
    drawBox(0.78f,1.30f,-0.72f,0.72f,0.58f,0.60f,
            0.36f,0.22f,0.12f,18.0f,0.12f);

    // red-cross sign on the front-facing end
    drawBox(0.0f,2.22f,-1.10f,0.82f,0.78f,0.06f,
            0.88f,0.84f,0.75f,8.0f,0.0f);
    drawBox(0.0f,2.22f,-1.145f,0.14f,0.56f,0.02f,
            0.68f,0.06f,0.05f,5.0f,0.0f);
    drawBox(0.0f,2.22f,-1.145f,0.56f,0.14f,0.02f,
            0.68f,0.06f,0.05f,5.0f,0.0f);

    glPopMatrix();

    drawArtworkLabel("FIELD MEDICAL UNIT",x,0.78f,z-1.48f,180);
}

void drawDocumentArchiveTable(float x,float z,float rotY) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);
    drawTexturedBox(0,0.66f,0,2.8f,1.32f,1.65f,texWood,34);
    drawBox(0,1.36f,0,2.30f,0.08f,1.10f,0.72f,0.67f,0.54f,20,0.12f);
    for(int i=0;i<5;i++) drawBox(-0.62f,1.42f,-0.35f+i*0.17f,0.90f,0.015f,0.022f,0.18f,0.15f,0.10f,8,0.05f);
    drawCylinderBetween(Vec3(0.70f,1.45f,0.24f),Vec3(1.00f,1.72f,0.34f),0.028f,0.60f,0.45f,0.16f);
    glPopMatrix();
    drawArtworkLabel("WAR DOCUMENT ARCHIVE",x,0.78f,z+1.00f,180);
}

void drawResistanceCampDisplay(float x,float z) {
    glPushMatrix(); glTranslatef(x,0,z);
    for(float dx: {-0.72f,0.72f}) for(int row=0;row<2;row++)
        drawBox(dx,0.35f+row*0.34f,0.28f,1.25f,0.58f,0.62f,0.29f,0.24f,0.15f,25,0.16f);
    drawBox(0,0.35f,-0.40f,1.60f,0.58f,0.80f,0.33f,0.28f,0.18f,25,0.16f);
    drawCylinderBetween(Vec3(-0.85f,1.45f,0.08f),Vec3(0.70f,2.42f,0.08f),0.026f,0.12f,0.11f,0.08f);
    drawBox(0.75f,2.43f,0.08f,0.18f,0.06f,0.06f,0.10f,0.09f,0.07f,25,0.30f);
    glPopMatrix();
    drawArtworkLabel("FIELD CAMP & EQUIPMENT",x,0.78f,z+1.10f,180);
}

void drawFieldWeaponsDisplay(float x,float z) {
    glPushMatrix(); glTranslatef(x,0,z);
    drawTexturedBox(0,0.52f,0,3.25f,1.04f,1.95f,texWood,34);
    // Display-only silhouettes: no moving or usable mechanisms.
    drawCylinderBetween(Vec3(-1.02f,1.26f,0.25f),Vec3(0.28f,1.50f,0.25f),0.045f,0.20f,0.18f,0.10f);
    drawBox(0.42f,1.52f,0.25f,0.30f,0.10f,0.10f,0.22f,0.19f,0.15f,28,0.30f);
    drawCylinderBetween(Vec3(-0.22f,1.23f,-0.32f),Vec3(0.92f,1.45f,-0.32f),0.040f,0.19f,0.17f,0.10f);
    drawBox(1.04f,1.46f,-0.32f,0.28f,0.08f,0.10f,0.24f,0.21f,0.16f,28,0.30f);
    drawScaledSphere(-0.96f,1.32f,-0.52f,0.58f,0.28f,0.46f,0.19f,0.25f,0.20f,28,0.32f);
    drawBox(0.84f,1.18f,0.52f,0.62f,0.50f,0.48f,0.30f,0.24f,0.14f,22,0.22f);
    drawBox(0.84f,1.24f,0.78f,0.26f,0.08f,0.04f,0.64f,0.52f,0.30f,12,0.16f);
    glPopMatrix();
    drawArtworkLabel("WEAPONS & FIELD EQUIPMENT",x,0.80f,z+1.25f,180);
}

void drawVictoryArchiveDisplay(float x,float z) {
    glPushMatrix(); glTranslatef(x,0,z);
    drawTexturedBox(0,0.52f,0,2.8f,1.04f,1.55f,texWood,34);
    drawBox(0,1.20f,0,1.95f,0.06f,1.05f,0.72f,0.66f,0.50f,20,0.08f);
    drawBox(-0.55f,1.43f,0.05f,0.42f,0.08f,0.74f,0.24f,0.20f,0.14f,20,0.25f);
    drawBox(0.30f,1.34f,0.08f,0.60f,0.04f,0.82f,0.78f,0.72f,0.56f,18,0.08f);
    drawBangladeshFlag(0.78f,0.05f,2.2f,0.32f);
    glPopMatrix();
    drawArtworkLabel("VICTORY ARCHIVE",x,0.78f,z+1.00f,180);
}


void drawMuseumNamePlate() {
    glPushMatrix();
    glTranslatef(0,7.05f,36.10f);
    drawBox(0,0,0,18.5f,2.20f,0.22f,0.93f,0.90f,0.80f,26,0.26f);
    drawBox(0,0,-0.13f,17.9f,1.64f,0.04f,0.06f,0.20f,0.13f,20,0.35f);
    glDisable(GL_LIGHTING);
    glColor3f(0.05f,0.09f,0.06f);
    glPushMatrix();
    glTranslatef(-7.75f,0.30f,0.125f);
    glScalef(0.00168f,0.00168f,0.00168f);
    const std::string title="BANGLADESH LIBERATION WAR MUSEUM";
    for(char c:title) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glColor3f(0.67f,0.06f,0.05f);
    glPushMatrix();
    glTranslatef(-2.15f,-0.48f,0.125f);
    glScalef(0.00150f,0.00150f,0.00150f);
    const std::string year="1971";
    for(char c:year) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// ================================================================
// Exterior entrance / plaza
// ================================================================

void drawTicketCounter(float x,float z) {
    glPushMatrix();
    glTranslatef(x,0,z);

    // Compact outdoor ticket booth with a clear front service window.
    drawBox(0,1.55f,0,5.0f,3.10f,2.55f,0.56f,0.48f,0.39f,32,0.28f);
    drawBox(0,3.18f,0,5.30f,0.24f,2.82f,0.20f,0.13f,0.055f,55,0.55f);
    drawBox(0,2.05f,1.30f,3.30f,1.15f,0.12f,0.08f,0.13f,0.15f,70,0.7f);
    drawBox(0,1.78f,1.42f,3.05f,0.08f,0.18f,0.72f,0.57f,0.22f,45,0.5f);

    // Lower service counter and payment/device shelf.
    drawBox(0,0.93f,1.34f,4.15f,0.18f,0.34f,0.34f,0.24f,0.10f,45,0.38f);
    drawBox(-0.72f,1.15f,1.53f,0.42f,0.22f,0.18f,0.08f,0.09f,0.10f,60,0.62f);
    drawBox(0.72f,1.15f,1.53f,0.42f,0.22f,0.18f,0.08f,0.09f,0.10f,60,0.62f);

    // Service awning + illuminated title.
    drawBox(0,3.72f,1.10f,5.70f,0.18f,1.05f,0.46f,0.31f,0.08f,50,0.5f);
    drawRoomSign("TICKET COUNTER",0,3.48f,1.66f,0,4.75f);
    drawRoomSign("ENTRANCE TICKETS",0,2.55f,1.58f,0,3.90f);

    // Small queue rail keeps the forecourt organised without filling it with objects.
    drawCylinderY(-3.05f,0.02f,0.95f,0.065f,1.05f,0.18f,0.16f,0.13f,45,0.45f);
    drawCylinderY( 3.05f,0.02f,0.95f,0.065f,1.05f,0.18f,0.16f,0.13f,45,0.45f);
    drawCylinderBetween(Vec3(-3.05f,0.90f,0.95f),Vec3(3.05f,0.90f,0.95f),0.028f,0.58f,0.30f,0.08f);
    glPopMatrix();
}

void drawVisitorBench(float x,float z,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);
    drawBox(0,0.55f,0,3.4f,0.16f,0.62f,0.34f,0.23f,0.12f,28,0.22f);
    drawBox(0,1.02f,-0.24f,3.4f,0.92f,0.14f,0.30f,0.20f,0.10f,28,0.22f);
    drawBox(-1.28f,0.24f,0,0.18f,0.48f,0.46f,0.18f,0.16f,0.14f,32,0.25f);
    drawBox( 1.28f,0.24f,0,0.18f,0.48f,0.46f,0.18f,0.16f,0.14f,32,0.25f);
    glPopMatrix();
}

void drawVisitorInfoTotem(float x,float z) {
    glPushMatrix();
    glTranslatef(x,0,z);
    drawTexturedBox(0,0.42f,0,0.85f,0.84f,0.85f,texWood,30);
    drawBox(0,2.08f,0,0.20f,2.50f,0.20f,0.18f,0.16f,0.14f,40,0.34f);
    drawBox(0,3.22f,0.02f,2.20f,1.25f,0.18f,0.07f,0.09f,0.11f,65,0.75f);
    drawRoomSign("VISITOR INFO",0,3.28f,0.14f,0,2.05f);
    glPopMatrix();
}

void drawDistantExteriorView() {
    // A far landscape panel acts like a simple sky/environment backdrop.  It is
    // placed beyond the playable forecourt, so from inside Level 2 it reads as
    // a distant outdoor scene rather than a wall immediately outside the glass.
    glPushMatrix();
    glTranslatef(0.0f,0.0f,0.0f);

    // Distant Bangladesh village/sky texture (already generated procedurally).
    drawTexturedBox(0.0f,12.2f,82.0f,54.0f,24.0f,0.12f,texVillage,8);

    // Green strip and a row of tall trees add depth/parallax in front of it.
    drawBox(0.0f,-0.22f,73.2f,54.0f,0.42f,14.5f,0.14f,0.31f,0.12f,10,0.05f);
    drawOutdoorTree(-20.0f,72.0f,1.35f);
    drawOutdoorTree(-10.5f,75.0f,1.18f);
    drawOutdoorTree( 10.5f,74.5f,1.22f);
    drawOutdoorTree( 20.0f,71.5f,1.38f);

    // Low boundary wall keeps the far area visually connected to the museum grounds.
    drawBox(0.0f,0.72f,67.6f,48.0f,1.20f,0.28f,0.54f,0.50f,0.43f,24,0.18f);

    glPopMatrix();
}

void drawExterior() {
    // Real scenery beyond the facade is visible through the Level-2 windows.
    drawDistantExteriorView();

    // Step 12: much deeper forecourt so the opening camera shows the complete
    // two-storey facade instead of starting almost at the entrance.
    drawTexturedBox(0,-0.28f,51.5f,54.0f,0.50f,31.0f,texFloor,38);
    drawTexturedBox(0,0.01f,51.5f,7.0f,0.06f,31.0f,texCarpet,10);

    // Entrance facade columns and canopy
    drawColumn(-8.0f,37.0f); drawColumn(8.0f,37.0f);
    drawColumn(-12.5f,37.0f); drawColumn(12.5f,37.0f);
    drawBox(0,8.95f,37.0f,30.0f,0.55f,2.2f,0.72f,0.70f,0.64f,45,0.5f);
    drawBox(0,9.38f,36.65f,31.5f,0.25f,2.5f,0.54f,0.40f,0.13f,55,0.65f);

    // External museum identity
    drawMuseumNamePlate();
    drawBangladeshFlag(-10.8f,39.5f,5.1f,0.90f);
    drawBangladeshFlag(10.8f,39.5f,5.1f,0.90f);

    // Garden planters and lamps
    // Keep greenery asymmetrical and sparse instead of repeating the same plant cluster.
    drawPlant(-20.5f,42.0f,1.0f);
    drawPlant( 20.5f,42.0f,1.0f);
    drawPlant(-15.5f,63.0f,0.72f);
    drawPlant( 15.5f,63.0f,0.72f);

    // Large outdoor Shaheed Minar memorial court.  Moving it from the interior
    // gives this culturally important landmark the scale and breathing room it needs.
    drawTexturedBox(-15.8f,0.02f,55.3f,13.2f,0.10f,9.4f,texFloor,36);
    drawBox(-15.8f,0.10f,55.3f,12.5f,0.12f,8.7f,0.70f,0.68f,0.62f,34,0.28f);
    glPushMatrix();
        glTranslatef(-15.8f,0.10f,55.3f);
        glScalef(1.55f,1.55f,1.55f);
        drawShaheedMinarMemorial(0.0f,0.0f,1.0f,0.0f);
    glPopMatrix();

    // A few visitors gather around the memorial and entrance area.
    drawMuseumVisitor(-9.2f,55.0f,0.90f,255);
    drawMuseumVisitor(-20.8f,61.0f,0.86f,120);
    drawMuseumVisitor(-7.0f,47.8f,0.84f,170);
    drawMuseumVisitor( 6.8f,51.0f,0.88f,-165);
    drawMuseumVisitor( 14.5f,63.0f,0.82f,-145);
    drawMuseumVisitor(-2.8f,57.0f,0.84f,190);
    drawMuseumVisitor( 2.8f,59.5f,0.86f,165);
    drawMuseumVisitor(-21.0f,47.5f,0.80f,35);
    drawMuseumVisitor( 20.0f,53.5f,0.82f,-65);

    // New visitor-services zone: one ticket counter, one information totem and one bench.
    drawTicketCounter(12.8f,49.0f);
    drawVisitorInfoTotem(-12.8f,49.2f);
    drawVisitorBench(17.0f,58.8f,180.0f);
    // Entrance lighting poles remain; remove the two oversized globe elements
    // closest to the main entrance because they clutter the facade view.
    for(float z : {46.0f,57.0f}) for(float x: {-10.0f,10.0f}){
        drawCylinderY(x,0.0f,z,0.10f,3.1f,0.16f,0.16f,0.17f,40,0.5f);
        if(z > 46.5f)
            drawSphere(x,3.15f,z,0.28f,1.0f,0.80f,0.36f,70,0.9f);
    }

    // Memorial-style entrance blocks replace the unrelated art sculptures.
    drawTexturedBox(-18.0f,0.55f,37.8f,2.0f,1.1f,2.0f,texFloor,36);
    drawBox(-18.0f,1.55f,37.55f,1.35f,0.16f,0.22f,0.60f,0.06f,0.04f,25,0.25f);
    drawTexturedBox(18.0f,0.55f,37.8f,2.0f,1.1f,2.0f,texFloor,36);
    drawBox(18.0f,1.55f,37.55f,1.35f,0.16f,0.22f,0.60f,0.06f,0.04f,25,0.25f);
}



// ================================================================
// Lighting
// ================================================================

void configureCeilingSpot(GLenum id,float x,float z,
                          float r,float g,float b,float cutoff=58.0f,float y=8.85f) {
    glEnable(id);
    GLfloat pos[]  = {x,y,z,1.0f};
    GLfloat amb[]  = {r*0.035f,g*0.035f,b*0.035f,1.0f};
    GLfloat dif[]  = {r,g,b,1.0f};
    GLfloat spec[] = {std::min(1.0f,r+0.08f),std::min(1.0f,g+0.08f),std::min(1.0f,b+0.08f),1.0f};
    GLfloat dir[]  = {0.0f,-1.0f,0.0f};
    glLightfv(id,GL_POSITION,pos);
    glLightfv(id,GL_AMBIENT,amb);
    glLightfv(id,GL_DIFFUSE,dif);
    glLightfv(id,GL_SPECULAR,spec);
    glLightfv(id,GL_SPOT_DIRECTION,dir);
    glLightf(id,GL_SPOT_CUTOFF,cutoff);
    glLightf(id,GL_SPOT_EXPONENT,12.0f);
    glLightf(id,GL_CONSTANT_ATTENUATION,0.82f);
    glLightf(id,GL_LINEAR_ATTENUATION,0.022f);
    glLightf(id,GL_QUADRATIC_ATTENUATION,0.0025f);
}

void setupLights() {
    glEnable(GL_LIGHTING);

    // Light 0: soft neutral directional fill. This prevents hard black areas
    // while still allowing the local gallery spotlights to shape the scene.
    if(lightsOn[0]) glEnable(GL_LIGHT0); else glDisable(GL_LIGHT0);
    GLfloat L0pos[] = {-0.25f,1.0f,0.15f,0.0f};
    GLfloat L0amb[] = {0.10f,0.10f,0.105f,1.0f};
    GLfloat L0dif[] = {0.56f,0.55f,0.52f,1.0f};
    GLfloat L0spec[]= {0.60f,0.59f,0.56f,1.0f};
    glLightfv(GL_LIGHT0,GL_POSITION,L0pos);
    glLightfv(GL_LIGHT0,GL_AMBIENT,L0amb);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,L0dif);
    glLightfv(GL_LIGHT0,GL_SPECULAR,L0spec);

    const bool upperLighting = (cameraPos.y-EYE_HEIGHT) > SECOND_FLOOR_Y*0.62f;

    // Light 1: warm pool. It follows the visitor upstairs so Level 2 is not dim.
    if(lightsOn[1]) glEnable(GL_LIGHT1); else glDisable(GL_LIGHT1);
    GLfloat L1pos[] = {0.0f,upperLighting?18.25f:8.55f,upperLighting?18.0f:29.0f,1.0f};
    GLfloat L1amb[] = {0.035f,0.025f,0.015f,1.0f};
    GLfloat L1dif[] = {1.0f,0.78f,0.48f,1.0f};
    GLfloat L1spec[]= {1.0f,0.92f,0.72f,1.0f};
    glLightfv(GL_LIGHT1,GL_POSITION,L1pos);
    glLightfv(GL_LIGHT1,GL_AMBIENT,L1amb);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,L1dif);
    glLightfv(GL_LIGHT1,GL_SPECULAR,L1spec);
    glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,0.78f);
    glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,0.020f);
    glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,0.0018f);

    // Light 2: cooler rear pool; also shifts to the upper common hall when needed.
    if(lightsOn[2]) glEnable(GL_LIGHT2); else glDisable(GL_LIGHT2);
    GLfloat L2pos[] = {0.0f,upperLighting?18.20f:8.45f,upperLighting?-18.0f:-29.0f,1.0f};
    GLfloat L2amb[] = {0.025f,0.030f,0.040f,1.0f};
    GLfloat L2dif[] = {0.66f,0.75f,0.92f,1.0f};
    GLfloat L2spec[]= {0.78f,0.84f,1.0f,1.0f};
    glLightfv(GL_LIGHT2,GL_POSITION,L2pos);
    glLightfv(GL_LIGHT2,GL_AMBIENT,L2amb);
    glLightfv(GL_LIGHT2,GL_DIFFUSE,L2dif);
    glLightfv(GL_LIGHT2,GL_SPECULAR,L2spec);
    glLightf(GL_LIGHT2,GL_CONSTANT_ATTENUATION,0.78f);
    glLightf(GL_LIGHT2,GL_LINEAR_ATTENUATION,0.020f);
    glLightf(GL_LIGHT2,GL_QUADRATIC_ATTENUATION,0.0018f);

    // Lights 3-7: actual downward gallery spotlights. Fixed-function OpenGL
    // guarantees at least eight lights, so these work alongside LIGHT0-2.
    // F1 acts as the main gallery-light switch for this group.
    if(lightsOn[0]) {
        if(upperLighting) {
            // Five real spotlights across the single open-plan Level 2 hall.
            configureCeilingSpot(GL_LIGHT3,-11.0f, 16.0f,0.96f,0.78f,0.50f,60.0f,18.35f);
            configureCeilingSpot(GL_LIGHT4, 11.0f, 16.0f,0.70f,0.82f,1.00f,60.0f,18.35f);
            configureCeilingSpot(GL_LIGHT5,  0.0f,  0.0f,0.74f,0.84f,1.00f,62.0f,18.35f);
            configureCeilingSpot(GL_LIGHT6,-11.0f,-16.0f,1.00f,0.68f,0.38f,60.0f,18.35f);
            configureCeilingSpot(GL_LIGHT7, 11.0f,-16.0f,0.88f,0.64f,1.00f,60.0f,18.35f);
        } else {
            configureCeilingSpot(GL_LIGHT3,-15.0f, 16.0f,0.96f,0.84f,0.62f);
            configureCeilingSpot(GL_LIGHT4, 15.0f, 16.0f,0.96f,0.84f,0.62f);
            configureCeilingSpot(GL_LIGHT5,-15.0f, -1.0f,0.96f,0.86f,0.70f);
            configureCeilingSpot(GL_LIGHT6, 15.0f, -1.0f,0.96f,0.86f,0.70f);
            configureCeilingSpot(GL_LIGHT7,  0.0f,-17.0f,0.92f,0.78f,0.60f,64.0f);
        }
    } else {
        glDisable(GL_LIGHT3); glDisable(GL_LIGHT4); glDisable(GL_LIGHT5);
        glDisable(GL_LIGHT6); glDisable(GL_LIGHT7);
    }

    // Visible bulbs follow the currently active floor.
    if(upperLighting) {
        drawGlowSphere(0,18.25f,18.0f,0.20f,0.20f,0.20f,1.0f,0.78f,0.36f,0.42f);
        drawGlowSphere(0,18.20f,-18.0f,0.20f,0.20f,0.20f,0.62f,0.74f,1.0f,0.34f);
    } else {
        drawGlowSphere(0,8.55f,29.0f,0.20f,0.20f,0.20f,1.0f,0.78f,0.36f,0.42f);
        drawGlowSphere(0,8.45f,-29.0f,0.20f,0.20f,0.20f,0.62f,0.74f,1.0f,0.34f);
    }
}


// ================================================================
// Camera and HUD
// ================================================================

Vec3 cameraForward() {
    float yr = cameraYaw*PI/180.0f;
    float pr = cameraPitch*PI/180.0f;
    return normalizeVec(Vec3(std::cos(yr)*std::cos(pr), std::sin(pr), std::sin(yr)*std::cos(pr)));
}

Vec3 cameraRight() {
    Vec3 f=cameraForward();
    return normalizeVec(Vec3(-f.z,0,f.x));
}

void applyCamera() {
    if(overviewCamera) {
        gluLookAt(0.0,62.0,76.0, 0.0,8.0,0.0, 0.0,1.0,0.0);
    } else {
        Vec3 f=cameraForward();
        Vec3 target=cameraPos+f;
        gluLookAt(cameraPos.x,cameraPos.y,cameraPos.z,
                  target.x,target.y,target.z,
                  0,1,0);
    }
}

void drawBitmapText(int x,int y,const std::string& s,void* font=GLUT_BITMAP_8_BY_13) {
    glRasterPos2i(x,y);
    for(char c:s) glutBitmapCharacter(font,c);
}

// Return the nearest major exhibit that is close enough to inspect.
// Ground-floor transform offsets are included so the E-key card still follows
// an exhibit after the teacher/demo transformations are shown.
int nearestExhibit(float maxDistance=6.4f) {
    const bool upstairs=(cameraPos.y-EYE_HEIGHT)>SECOND_FLOOR_Y*0.62f;
    float best=maxDistance;
    int bestId=-1;

    for(int i=0;i<7;i++) {
        if(exhibitInfoTable[i].upperFloor!=upstairs) continue;
        float ex=exhibitInfoTable[i].x;
        float ez=exhibitInfoTable[i].z;
        if(i<4) { ex+=artXform[i].tx; ez+=artXform[i].tz; }
        float dx=cameraPos.x-ex;
        float dz=cameraPos.z-ez;
        float d=std::sqrt(dx*dx+dz*dz);
        if(d<best) { best=d; bestId=i; }
    }
    return bestId;
}

void drawHudPanel(float x0,float y0,float x1,float y1,float alpha=0.82f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.035f,0.045f,0.065f,alpha);
    glBegin(GL_QUADS);
    glVertex2f(x0,y0); glVertex2f(x1,y0); glVertex2f(x1,y1); glVertex2f(x0,y1);
    glEnd();
    glColor4f(0.88f,0.66f,0.22f,0.95f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0,y0); glVertex2f(x1,y0); glVertex2f(x1,y1); glVertex2f(x0,y1);
    glEnd();
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

std::string currentRoomName() {
    float floorY=cameraPos.y-EYE_HEIGHT;
    if(floorY > SECOND_FLOOR_Y-1.2f) return "Level 2 - Liberation War Exhibition (1971)";
    if(floorY > 0.85f) return "Grand Staircase";
    if(cameraPos.z > 36.2f) return "Entrance Plaza / Forecourt";
    if(cameraPos.z > 24.0f) return "Grand Lobby";
    if(cameraPos.z < -28.0f) return "Remembrance Hall";
    if(std::fabs(cameraPos.x) < 6.0f) return "Main Liberation War Corridor";
    if(cameraPos.x < -6.0f){
        if(cameraPos.z > 8.0f) return "Road to Independence";
        if(cameraPos.z > -10.0f) return "People & Resistance";
        return "Civilian Experience";
    }
    if(cameraPos.z > 8.0f) return "War Organization";
    if(cameraPos.z > -10.0f) return "War Archive";
    return "War Artifacts & Documents";
}

void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0,gWinW,0,gWinH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    // Small first-person crosshair
    glColor3f(0.95f,0.85f,0.48f);
    drawBitmapText(gWinW/2-3,gWinH/2-4,"+",GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.95f,0.95f,0.95f);
    drawBitmapText(18,gWinH-26,std::string("Room: ")+currentRoomName(),GLUT_BITMAP_HELVETICA_12);

    const int nearId=nearestExhibit();
    if(nearId>=0) {
        // Compact contextual prompt: no new physical sign/object is added to the room.
        std::string prompt=std::string("Nearby: ")+exhibitInfoTable[nearId].title+"   [E] exhibit info";
        int pw=std::min(gWinW-40,520);
        drawHudPanel(18,34,(float)(18+pw),68,0.72f);
        glColor3f(0.98f,0.90f,0.68f);
        drawBitmapText(32,47,prompt,GLUT_BITMAP_HELVETICA_12);
    }

    // Full information card appears only on demand. It behaves like a smart
    // museum interpretation screen while leaving the 3D gallery uncluttered.
    if(showExhibitInfo && exhibitInfoId>=0 && exhibitInfoId<7) {
        const ExhibitInfo& e=exhibitInfoTable[exhibitInfoId];
        const float cardW=std::min(720.0f,(float)gWinW-48.0f);
        const float x0=(gWinW-cardW)*0.5f;
        const float x1=x0+cardW;
        const float y0=88.0f, y1=204.0f;
        drawHudPanel(x0,y0,x1,y1,0.90f);
        glColor3f(1.0f,0.78f,0.28f);
        drawBitmapText((int)x0+20,(int)y1-30,e.title,GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.80f,0.84f,0.90f);
        drawBitmapText((int)x0+20,(int)y1-51,e.collection,GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.96f,0.96f,0.94f);
        drawBitmapText((int)x0+20,(int)y1-75,e.line1,GLUT_BITMAP_HELVETICA_12);
        drawBitmapText((int)x0+20,(int)y1-94,e.line2,GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.72f,0.76f,0.82f);
        drawBitmapText((int)x1-145,(int)y0+10,"E = close",GLUT_BITMAP_HELVETICA_12);
    }

    if(showHelp){
        const char* names[] = {"7 March Memorial","Freedom Fighter Memorial","Civilian Memory Memorial","Victory & Independence Memorial"};
        drawBitmapText(18,gWinH-48,"W/A/S/D = walk like first person | Mouse = look | V = overview | M = release/capture mouse",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-68,"Walk close to a major exhibit and press E for Smart Museum information.",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-88,"1-4 select section | HOLD J/L = X | I/K = Z | U/O = Y | R/T = rotate | +/- = scale",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-108,"0 reset selected | F1/F2/F3 lights | P animation | H help | ESC exit",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-130,std::string("Selected: ")+names[selectedArt]+" ",GLUT_BITMAP_HELVETICA_12);
        char tf[180];
        std::snprintf(tf,sizeof(tf),"Transform  X %.2f  Y %.2f  Z %.2f  Rot %.0f deg  Scale %.2f",
                      artXform[selectedArt].tx,artXform[selectedArt].ty,artXform[selectedArt].tz,
                      artXform[selectedArt].rotY,artXform[selectedArt].scale);
        drawBitmapText(18,gWinH-150,tf,GLUT_BITMAP_HELVETICA_12);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ================================================================
// Rendering
// ================================================================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Stable fixed-function color/material state for every frame.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glColor4f(1.0f,1.0f,1.0f,1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    applyCamera();

    setupLights();
    drawExterior();
    drawMuseumArchitecture();
    drawAdditionalGalleryObjects();
    // Selection floor gizmo removed; statue transformations remain active.

    drawHUD();
    glutSwapBuffers();
}

void reshape(int w,int h) {
    gWinW=std::max(1,w); gWinH=std::max(1,h);
    glViewport(0,0,gWinW,gWinH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(68.0, double(gWinW)/double(gWinH), 0.06, 180.0);
    glMatrixMode(GL_MODELVIEW);
}

// ================================================================
// Controls
// ================================================================

void resetSelectedObject() {
    artXform[selectedArt]=ObjectTransform();
}

void keyboardDown(unsigned char key,int,int) {
    keyState[(unsigned char)key]=true;

    switch(key) {
        case 27: std::exit(0); break;
        case 'h': case 'H': showHelp=!showHelp; break;
        case 'v': case 'V':
            overviewCamera=!overviewCamera;
            if(!overviewCamera && mouseLookEnabled) glutWarpPointer(gWinW/2,gWinH/2);
            break;
        case 'm': case 'M':
            mouseLookEnabled=!mouseLookEnabled;
            firstMouse=true;
            glutSetCursor(mouseLookEnabled ? GLUT_CURSOR_NONE : GLUT_CURSOR_INHERIT);
            if(mouseLookEnabled) glutWarpPointer(gWinW/2,gWinH/2);
            break;
        case 'p': case 'P': animateScene=!animateScene; break;
        case 'e': case 'E': {
            int nearId=nearestExhibit();
            if(showExhibitInfo) {
                showExhibitInfo=false;
                exhibitInfoId=-1;
            } else if(nearId>=0) {
                exhibitInfoId=nearId;
                showExhibitInfo=true;
            }
            break;
        }
        case '1': selectedArt=0; break;
        case '2': selectedArt=1; break;
        case '3': selectedArt=2; break;
        case '4': selectedArt=3; break;
        case '0': resetSelectedObject(); break;
        case '+': case '=': artXform[selectedArt].scale=clampf(artXform[selectedArt].scale+0.10f,0.45f,1.80f); break;
        case '-': case '_': artXform[selectedArt].scale=clampf(artXform[selectedArt].scale-0.10f,0.45f,1.80f); break;
        case 'j': case 'J': artXform[selectedArt].tx-=0.55f; break;
        case 'l': case 'L': artXform[selectedArt].tx+=0.55f; break;
        case 'i': case 'I': artXform[selectedArt].tz-=0.55f; break;
        case 'k': case 'K': artXform[selectedArt].tz+=0.55f; break;
        case 'u': case 'U': artXform[selectedArt].ty+=0.35f; break;
        case 'o': case 'O': artXform[selectedArt].ty-=0.35f; break;
        case 'r': case 'R': artXform[selectedArt].rotY+=12.0f; break;
        case 't': case 'T': artXform[selectedArt].rotY-=12.0f; break;
    }
    glutPostRedisplay();
}

void keyboardUp(unsigned char key,int,int) {
    keyState[(unsigned char)key]=false;
}

void specialDown(int key,int,int) {
    if(key==GLUT_KEY_F1) lightsOn[0]=!lightsOn[0];
    if(key==GLUT_KEY_F2) lightsOn[1]=!lightsOn[1];
    if(key==GLUT_KEY_F3) lightsOn[2]=!lightsOn[2];
    if(key==GLUT_KEY_UP) cameraPitch=clampf(cameraPitch+2.0f,-80.0f,80.0f);
    if(key==GLUT_KEY_DOWN) cameraPitch=clampf(cameraPitch-2.0f,-80.0f,80.0f);
    if(key==GLUT_KEY_LEFT) cameraYaw-=2.5f;
    if(key==GLUT_KEY_RIGHT) cameraYaw+=2.5f;
    glutPostRedisplay();
}

void mouseMotion(int x,int y) {
    if(!mouseLookEnabled || overviewCamera) return;
    int cx=gWinW/2, cy=gWinH/2;
    int dx=x-cx;
    int dy=cy-y;
    if(std::abs(dx)<=1 && std::abs(dy)<=1) return;
    cameraYaw += dx*mouseSensitivity;
    cameraPitch += dy*mouseSensitivity;
    cameraPitch=clampf(cameraPitch,-78.0f,78.0f);
    glutWarpPointer(cx,cy);
    glutPostRedisplay();
}

void mouseButton(int button,int state,int,int) {
    if(state!=GLUT_DOWN) return;
    if(button==3) moveSpeed=clampf(moveSpeed+0.8f,2.0f,18.0f); // wheel up
    if(button==4) moveSpeed=clampf(moveSpeed-0.8f,2.0f,18.0f); // wheel down
}

// ================================================================
// Animation / movement update
// ================================================================

int previousTime=0;

bool insideRectExpanded(float x,float z,float cx,float cz,float sx,float sz,float r) {
    return x>cx-sx*0.5f-r && x<cx+sx*0.5f+r && z>cz-sz*0.5f-r && z<cz+sz*0.5f+r;
}

bool inRange(float v,float a,float b) {
    if(a>b) std::swap(a,b);
    return v>=a && v<=b;
}

// Return the walking-surface height under a prospective X/Z position.
// currentSurface is required because ground floor and level 2 share many
// of the same X/Z coordinates.
float walkSurfaceHeight(float x,float z,float currentSurface) {
    const float half=STAIR_FLIGHT_W*0.5f+0.24f;

    // Sloped flights are evaluated before the broad landings. This avoids
    // height jumps where the landing overlaps the first/last tread.
    if(std::fabs(x-STAIR_FIRST_X)<=half && inRange(z,STAIR_Z_REAR,STAIR_Z_FRONT) &&
       currentSurface < STAIR_MID_Y+0.65f) {
        float t=(STAIR_Z_FRONT-z)/(STAIR_Z_FRONT-STAIR_Z_REAR);
        return clampf(t,0.0f,1.0f)*STAIR_MID_Y;
    }

    if(std::fabs(x-STAIR_SECOND_X)<=half && inRange(z,STAIR_Z_REAR,STAIR_Z_FRONT) &&
       currentSurface > STAIR_MID_Y-0.65f) {
        float t=(z-STAIR_Z_REAR)/(STAIR_Z_FRONT-STAIR_Z_REAR);
        return STAIR_MID_Y+clampf(t,0.0f,1.0f)*STAIR_MID_Y;
    }

    if(inRange(x,STAIR_LANDING_X_MIN,STAIR_LANDING_X_MAX) &&
       inRange(z,STAIR_LANDING_Z_MIN,STAIR_LANDING_Z_MAX) &&
       currentSurface < SECOND_FLOOR_Y-0.70f)
        return STAIR_MID_Y;

    if(inRange(x,15.30f,23.48f) && inRange(z,STAIR_TOP_Z_MIN,STAIR_TOP_Z_MAX) &&
       currentSurface > SECOND_FLOOR_Y-1.10f)
        return SECOND_FLOOR_Y;

    bool onMainUpper=(x<=15.25f && z>=-35.45f && z<=35.45f);
    bool onRightRearUpper=(x>15.25f && x<23.48f && z<STAIR_LANDING_Z_MIN+0.25f && z>=-35.45f);
    bool onFrontLanding=(x>15.25f && x<23.48f && z>=STAIR_TOP_Z_MIN && z<=35.45f);
    if(currentSurface>SECOND_FLOOR_Y*0.62f &&
       (onMainUpper || onRightRearUpper || onFrontLanding))
        return SECOND_FLOOR_Y;

    return GROUND_FLOOR_Y;
}

bool cameraBlocked(float x,float z,float surfaceY) {
    const float r=0.34f;
    const bool upstairs=surfaceY>SECOND_FLOOR_Y*0.62f;

    // World / building boundary. Outside plaza is allowed only in front.
    if(z>67.5f || z<-35.45f || x<-26.0f || x>26.0f) return true;
    if(z<35.8f && (x<-23.42f || x>23.42f)) return true;

    // Outdoor Shaheed Minar memorial court occupies the left side of the forecourt.
    if(!upstairs && insideRectExpanded(x,z,-15.8f,55.3f,11.8f,7.7f,r)) return true;

    if(upstairs) {
        // Level 2 has a full front wall; there is no upper exterior doorway.
        if(insideRectExpanded(x,z,0.0f,36.0f,48.0f,0.50f,r)) return true;

        // Safety barrier around the stairwell opening.  Leave the top of the
        // second flight open so the visitor can descend naturally.
        if(insideRectExpanded(x,z,15.35f,28.90f,0.16f,7.55f,r)) return true;
        if(insideRectExpanded(x,z,19.40f,25.15f,8.10f,0.16f,r)) return true;
        if(insideRectExpanded(x,z,17.40f,32.65f,4.10f,0.16f,r)) return true;
        if(insideRectExpanded(x,z,23.00f,32.65f,0.90f,0.16f,r)) return true;

        // Open-plan Level 2 exhibit footprints: visitors can get close, but not
        // walk through the central sculpture or the four pedestal artworks.
        if(insideRectExpanded(x,z,0.0f,  0.0f,5.7f,5.7f,r)) return true;
        if(insideRectExpanded(x,z,0.0f, 18.0f,5.2f,5.2f,r)) return true;
        if(insideRectExpanded(x,z,0.0f,-18.0f,5.2f,5.2f,r)) return true;
        if(insideRectExpanded(x,z,-11.0f,14.0f,2.5f,2.5f,r)) return true;
        if(insideRectExpanded(x,z, 11.0f,14.0f,2.5f,2.5f,r)) return true;
        if(insideRectExpanded(x,z,-11.0f,-15.0f,2.5f,2.5f,r)) return true;
        if(insideRectExpanded(x,z, 11.0f,-15.0f,2.5f,2.5f,r)) return true;
        // New large equipment displays and compact dioramas.
        if(insideRectExpanded(x,z,-15.5f,-28.4f,5.2f,3.7f,r)) return true;
        if(insideRectExpanded(x,z, 15.0f,-28.2f,5.4f,3.7f,r)) return true;
        if(insideRectExpanded(x,z,-18.0f, 22.0f,3.2f,2.6f,r)) return true;
        if(insideRectExpanded(x,z, 17.6f,  0.0f,3.8f,3.0f,r)) return true;
        return false;
    }

    // Ground-floor front wall; central doorway remains open.
    if(insideRectExpanded(x,z,-14.25f,36.0f,19.5f,0.50f,r)) return true;
    if(insideRectExpanded(x,z, 14.25f,36.0f,19.5f,0.50f,r)) return true;

    // Side corridor walls with 3 gaps per side.
    const float xs[2]={-6.0f,6.0f};
    const float segZ[4]={21.25f,7.50f,-9.00f,-21.75f};
    const float segL[4]={5.5f,12.0f,11.0f,4.5f};
    for(float wx:xs) for(int i=0;i<4;i++)
        if(insideRectExpanded(x,z,wx,segZ[i],0.45f,segL[i],r)) return true;

    // Horizontal room separators in the side wings.
    if(insideRectExpanded(x,z,-15.0f, 8.0f,18.0f,0.45f,r)) return true;
    if(insideRectExpanded(x,z, 15.0f, 8.0f,18.0f,0.45f,r)) return true;
    if(insideRectExpanded(x,z,-15.0f,-10.0f,18.0f,0.45f,r)) return true;
    if(insideRectExpanded(x,z, 15.0f,-10.0f,18.0f,0.45f,r)) return true;

    // Added room-scale exhibits: keep the first-person camera out of the models.
    if(insideRectExpanded(x,z,-19.1f,17.1f,3.2f,2.6f,r)) return true;
    if(insideRectExpanded(x,z, 19.0f,-4.0f,3.8f,3.0f,r)) return true;

    return false;
}

void update() {
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=(now-previousTime)/1000.0f;
    previousTime=now;
    dt=std::min(dt,0.05f);

    if(animateScene) {
        centralRotation += 28.0f*dt;
        fanRotation += 165.0f*dt;
        if(centralRotation>360) centralRotation-=360;
        if(fanRotation>360) fanRotation-=360;
    }

    // Step 8: transform controls also work continuously while a key is held.
    // This makes the required translation/rotation/scaling easy to demonstrate.
    ObjectTransform& ot=artXform[selectedArt];
    const float tr=2.25f*dt;
    const float vr=1.55f*dt;
    const float rr=72.0f*dt;
    if(keyState[(unsigned char)'j'] || keyState[(unsigned char)'J']) ot.tx-=tr;
    if(keyState[(unsigned char)'l'] || keyState[(unsigned char)'L']) ot.tx+=tr;
    if(keyState[(unsigned char)'i'] || keyState[(unsigned char)'I']) ot.tz-=tr;
    if(keyState[(unsigned char)'k'] || keyState[(unsigned char)'K']) ot.tz+=tr;
    if(keyState[(unsigned char)'u'] || keyState[(unsigned char)'U']) ot.ty+=tr*0.70f;
    if(keyState[(unsigned char)'o'] || keyState[(unsigned char)'O']) ot.ty-=tr*0.70f;
    if(keyState[(unsigned char)'r'] || keyState[(unsigned char)'R']) ot.rotY+=rr;
    if(keyState[(unsigned char)'t'] || keyState[(unsigned char)'T']) ot.rotY-=rr;
    if(keyState[(unsigned char)'+'] || keyState[(unsigned char)'=']) ot.scale=clampf(ot.scale+vr,0.45f,1.80f);
    if(keyState[(unsigned char)'-'] || keyState[(unsigned char)'_']) ot.scale=clampf(ot.scale-vr,0.45f,1.80f);
    ot.ty=clampf(ot.ty,-0.70f,3.20f);

    if(!overviewCamera) {
        Vec3 f=cameraForward();
        f.y=0; f=normalizeVec(f);
        Vec3 r=cameraRight();
        r.y=0; r=normalizeVec(r);
        float s=moveSpeed*dt;
        Vec3 delta(0,0,0);
        if(keyState[(unsigned char)'w'] || keyState[(unsigned char)'W']) delta=delta+f*s;
        if(keyState[(unsigned char)'s'] || keyState[(unsigned char)'S']) delta=delta-f*s;
        if(keyState[(unsigned char)'a'] || keyState[(unsigned char)'A']) delta=delta-r*s;
        if(keyState[(unsigned char)'d'] || keyState[(unsigned char)'D']) delta=delta+r*s;

        // Step 10A: collision + walking-surface logic now supports two floors
        // and lets the camera physically climb the switchback staircase.
        float surfaceY=cameraPos.y-EYE_HEIGHT;

        float nx=cameraPos.x+delta.x;
        float sx=walkSurfaceHeight(nx,cameraPos.z,surfaceY);
        // Reject sudden large drops from an elevated landing/floor. Normal
        // stair descent changes height smoothly, but stepping into the void
        // would drop several world units in one movement.
        bool unsafeDropX=(surfaceY>0.80f && sx<surfaceY-0.70f);
        if(!unsafeDropX && !cameraBlocked(nx,cameraPos.z,sx)) {
            cameraPos.x=nx;
            surfaceY=sx;
        }

        float nz=cameraPos.z+delta.z;
        float sz=walkSurfaceHeight(cameraPos.x,nz,surfaceY);
        bool unsafeDropZ=(surfaceY>0.80f && sz<surfaceY-0.70f);
        if(!unsafeDropZ && !cameraBlocked(cameraPos.x,nz,sz)) {
            cameraPos.z=nz;
            surfaceY=sz;
        }

        cameraPos.y=surfaceY+EYE_HEIGHT;
    }

    // If the visitor leaves the exhibit area, close the contextual card so it
    // never blocks the view while walking through the rest of the museum.
    if(showExhibitInfo) {
        int nearId=nearestExhibit(8.2f);
        if(nearId!=exhibitInfoId) {
            showExhibitInfo=false;
            exhibitInfoId=-1;
        }
    }

    glutPostRedisplay();
}

// ================================================================
// OpenGL setup / main
// ================================================================

void initGL() {
    glClearColor(0.055f,0.065f,0.085f,1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    GLfloat globalAmbient[] = {0.135f,0.135f,0.145f,1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,globalAmbient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);

    // Startup-safe fixed-function material mode. This matches the earlier
    // versions that opened reliably on the user's Windows/freeglut setup.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glColor4f(1.0f,1.0f,1.0f,1.0f);

    createTextures();

    // Initial transformations: subtle rotation and scaling make the memorials
    // feel individually placed while keeping their original display zones.
    // Initial memorial orientations: both visitor-facing memorial groups
    // are turned 180 degrees as requested. Their existing fine-angle offsets
    // are preserved so the statues still have a natural presentation.
    artXform[0].rotY=173.0f;  artXform[0].scale=1.04f;
    artXform[1].rotY=188.0f;  artXform[1].scale=1.02f;
    artXform[2].rotY=-4.0f;  artXform[2].scale=1.02f;
    artXform[3].rotY= 0.0f;  artXform[3].scale=1.03f;

    previousTime=glutGet(GLUT_ELAPSED_TIME);
}

int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWinW,gWinH);
    glutInitWindowPosition(80,45);
    int museumWindow = glutCreateWindow("Bangladesh Liberation War Museum - 1971 | CSE 444");
    if(museumWindow <= 0) {
#ifdef _WIN32
        MessageBoxA(NULL,"OpenGL museum window could not be created.","Museum startup error",MB_OK|MB_ICONERROR);
#endif
        return 1;
    }

    initGL();
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(gWinW/2,gWinH/2);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouseButton);
    glutIdleFunc(update);

    glutMainLoop();
    return 0;
}
