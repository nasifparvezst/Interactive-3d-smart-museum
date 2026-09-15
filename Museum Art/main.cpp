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
// Interactive 3D Smart Museum & Art Gallery
// CSE 444 Computer Graphics Project
// ---------------------------------------------------------------
// Features implemented:
// - Large multi-room 3D museum + exterior entrance
// - Human-height first-person WASD + mouse camera with wall collision
// - Object translation, rotation and scaling
// - Step 9: redesigned natural-color tiger, elephant and spotted deer + cleaned benches
// - Step 10A: walkable grand staircase + true second floor shell, landing and railings
// - Step 10B: one large open-plan Level 2 Grand Exhibition Hall (no separate rooms)
// - Step 11: widened safe staircase/landings and readable black-on-ivory exhibit labels
// - Step 12: farther exterior start, doorway-overlap removal, redesigned dual-face illuminated clock,
//            high-contrast plinth names, two additional Level-2 feature sculptures, and stronger wildlife colors/shapes
// - Step 17: startup-safe Windows/freeglut material-state recovery
// - Step 13: Smart Museum interaction layer: proximity-based exhibit detection, E-key information cards,
//            room-aware visitor prompts, and non-cluttering digital interpretation for major exhibits
// - 8-source layered museum lighting (directional, point and gallery spotlights)
// - Ambient, diffuse and specular materials
// - Decorative marble floor inlays, room borders and corridor accents
// - Coffered ceiling details, recessed fixtures and wall wainscoting
// - Many procedural painting textures and wall decorations
// - Step 8 interaction/layout pass: reliable transform controls, overlap audit, high-contrast illuminated entrance clock
// - Museum-style non-occluding glass-frame artifact cases with varied exhibits
// - Reduced wall-side seating, flush wall mounting and unobstructed visitor circulation
// - Portrait-ready classical gallery frames for future real/user-supplied photos
// - Animated central sculpture and ceiling fans
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

// 0 tiger, 1 elephant, 2 deer, 3 abstract sculpture
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

// Entries 0-3 follow the transformable ground-floor exhibits.  Entries 4-6
// are the three large Level-2 centerpieces already present in the hall.
const ExhibitInfo exhibitInfoTable[7] = {
    {"ROYAL BENGAL TIGER","Wildlife Collection",
     "A full 3D wildlife study with Bengal coat colours and stripe details.",
     "Use 1 to select it; J/L, I/K, U/O, R/T and +/- demonstrate transforms.",
     -15.0f, 16.0f, false},
    {"ASIAN ELEPHANT","Wildlife Collection",
     "A large natural-grey elephant exhibit with trunk, tusks, ears and body detail.",
     "Walk around the plinth to inspect the model from different viewing coordinates.",
      15.0f, -1.0f, false},
    {"SPOTTED DEER","Wildlife Collection",
     "A chestnut-and-white spotted deer study with antlers and slender legs.",
     "The exhibit is positioned for close first-person viewing from several angles.",
       0.0f,-29.0f, false},
    {"KINETIC SCULPTURE","Modern Art Collection",
     "An animated abstract artwork demonstrating continuous rotational motion.",
     "Press P to pause/resume animation; key 4 selects it for transformations.",
     -15.0f, -1.0f, false},
    {"CELESTIAL GLOBE","Grand Exhibition Hall",
     "A large illuminated feature sculpture on the second-floor exhibition axis.",
     "Its form is designed to read clearly under the upper-gallery lighting system.",
       0.0f, 18.0f, true},
    {"AURORA ORBIT","Grand Exhibition Hall",
     "The central kinetic light sculpture and visual focus of the open Level-2 hall.",
     "Its animation and lighting create motion without adding extra floor clutter.",
       0.0f,  0.0f, true},
    {"CRYSTAL BLOOM","Grand Exhibition Hall",
     "A contemporary light-art centerpiece balancing the opposite end of the red floor.",
     "View it from close range or from the upper hall circulation path.",
       0.0f,-18.0f, true}
};

// ----------------------- Two-floor building ----------------------
// Ground floor is Y=0.  The upper-floor slab sits on top of the old
// ground-floor ceiling, so the original galleries remain unchanged.
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

void drawCylinderY(float x,float y,float z,float radius,float height,
                   float r,float g,float b,float shininess=24,float spec=0.3f) {
    static GLUquadric* q = nullptr;
    if (!q) {
        q = gluNewQuadric();
        gluQuadricNormals(q, GLU_SMOOTH);
    }
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(-90.0f,1,0,0); // GLU +Z -> +Y
    setMaterial(r,g,b,shininess,spec);
    gluCylinder(q, radius, radius, height, 20, 3);
    gluDisk(q, 0.0, radius, 20, 1);
    glTranslatef(0,0,height);
    gluDisk(q, 0.0, radius, 20, 1);
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

void createTextures() {
    texFloor = makeMarbleTexture();
    texWall = makeWallTexture();
    texWood = makeWoodTexture();
    texCeiling = makeCeilingTexture();
    texCarpet = makeCarpetTexture();
    texPainting1 = makePaintingTexture(0);
    texPainting2 = makePaintingTexture(1);
    texPainting3 = makePaintingTexture(2);
    texPainting4 = makePaintingTexture(3);
    texPainting5 = makePaintingTexture(4);
    texPainting6 = makePaintingTexture(5);
    texPainting7 = makePaintingTexture(6);
    texPainting8 = makePaintingTexture(7);
    texPainting9 = makePaintingTexture(8);
    texPainting10 = makePaintingTexture(9);
    texPainting11 = makePaintingTexture(10);
    texPainting12 = makePaintingTexture(11);
    texPainting13 = makePaintingTexture(12);
    texPainting14 = makePaintingTexture(13);
    texPortrait1 = makePaintingTexture(14);
    texPortrait2 = makePaintingTexture(15);
    texPortrait3 = makePaintingTexture(16);
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

void drawTigerWallRelief(float x,float y,float z,float rotY) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawTexturedBox(0,0,0,3.3f,3.5f,0.16f,texWood,35);
    glTranslatef(0,0,0.24f);
    drawScaledSphere(0,0.05f,0,0.95f,0.82f,0.34f,0.93f,0.43f,0.07f,50,0.45f);
    drawScaledSphere(0,-0.28f,0.28f,0.52f,0.30f,0.20f,0.94f,0.74f,0.44f,35,0.35f);
    drawConeY(-0.62f,0.62f,0,0.26f,0.50f,0.73f,0.26f,0.04f);
    drawConeY( 0.62f,0.62f,0,0.26f,0.50f,0.73f,0.26f,0.04f);
    drawScaledSphere(-0.31f,0.20f,0.31f,0.10f,0.08f,0.05f,0.03f,0.02f,0.01f,70,0.6f);
    drawScaledSphere( 0.31f,0.20f,0.31f,0.10f,0.08f,0.05f,0.03f,0.02f,0.01f,70,0.6f);
    for(int i=-2;i<=2;i++) drawBox(i*0.28f,0.58f-0.07f*std::abs(i),0.33f,0.10f,0.48f,0.05f,0.05f,0.035f,0.02f,18,0.1f);
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

void drawBench(float x,float z,float rotY=0) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);

    // Step 9 bench cleanup: simple all-wood museum bench.
    // The previous cushion/top layer could read as a stray white object under
    // strong lighting, so the seat is now one solid wooden surface only.
    const float wr=0.28f, wg=0.13f, wb=0.055f;
    const float wr2=0.20f, wg2=0.085f, wb2=0.030f;

    // Seat slab
    drawBox(0,0.78f,0,4.0f,0.28f,0.92f,wr,wg,wb,28,0.12f);

    // Backrest: three horizontal wooden slats, visually lighter and less bulky.
    drawBox(0,1.18f,0.40f,4.0f,0.20f,0.16f,wr,wg,wb,24,0.10f);
    drawBox(0,1.48f,0.40f,4.0f,0.20f,0.16f,wr,wg,wb,24,0.10f);
    drawBox(0,1.78f,0.40f,4.0f,0.20f,0.16f,wr,wg,wb,24,0.10f);

    // Four compact dark wooden legs.
    drawBox(-1.55f,0.36f, 0.25f,0.20f,0.72f,0.22f,wr2,wg2,wb2,22,0.08f);
    drawBox( 1.55f,0.36f, 0.25f,0.20f,0.72f,0.22f,wr2,wg2,wb2,22,0.08f);
    drawBox(-1.55f,0.36f,-0.25f,0.20f,0.72f,0.22f,wr2,wg2,wb2,22,0.08f);
    drawBox( 1.55f,0.36f,-0.25f,0.20f,0.72f,0.22f,wr2,wg2,wb2,22,0.08f);

    glPopMatrix();
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

void drawFloorRosette(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,0.075f,z);
    glScalef(scale,scale,scale);
    glRotatef(90,1,0,0);
    setMaterial(0.52f,0.38f,0.13f,65,0.70f);
    glutSolidTorus(0.08f,1.15f,18,48);
    setMaterial(0.22f,0.25f,0.29f,50,0.48f);
    glutSolidTorus(0.05f,0.72f,16,40);
    glPopMatrix();

    // compass-like center motif
    for(int i=0;i<4;i++) {
        glPushMatrix();
        glTranslatef(x,0.08f,z);
        glRotatef(i*45.0f,0,1,0);
        drawBox(0,0,0.62f*scale,0.14f*scale,0.08f,0.80f*scale,
                0.56f,0.43f,0.18f,58,0.55f);
        glPopMatrix();
    }
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

    // Feature motifs in the lobby and sculpture hall.
    drawFloorRosette(0,29.0f,1.05f);
    drawFloorRosette(0,-30.0f,0.88f);
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
    // Recessed navy "glass" window with a warm bronze frame.
    drawBox(x,y,z,4.15f,4.55f,0.12f,0.085f,0.12f,0.19f,65,0.55f);
    drawBox(x,y,z+0.08f,3.65f,4.05f,0.08f,0.045f,0.090f,0.16f,75,0.72f);
    drawBox(x,y,z+0.14f,0.10f,4.05f,0.06f,0.68f,0.50f,0.17f,60,0.62f);
    drawBox(x,y,z+0.14f,3.65f,0.10f,0.06f,0.68f,0.50f,0.17f,60,0.62f);
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
    const int posts=7;
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
    const int N=20;
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

    drawHorizontalRailingX(STAIR_LANDING_X_MIN+0.08f,STAIR_LANDING_X_MAX-0.08f,
                           STAIR_LANDING_Z_MIN+0.05f,STAIR_MID_Y);

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
    drawTexturedBox(0,14.75f, 36.0f,48.0f,9.50f,0.50f,texWall,10);

    // Second-floor roof and exterior crown line.
    drawTexturedBox(0,19.58f,0,48.0f,0.34f,72.0f,texCeiling,10);
    drawBox(0,19.30f,35.68f,47.5f,0.34f,0.22f,0.58f,0.43f,0.14f,55,0.65f);
    drawBox(0,10.15f,35.70f,47.5f,0.22f,0.18f,0.58f,0.43f,0.14f,55,0.65f);

    // Front upper windows make the two-storey facade obvious from outside.
    for(float x : {-18.0f,-11.0f,-4.0f,4.0f,11.0f,18.0f})
        drawUpperWindow(x,14.70f,36.30f);

    // Stairwell opening railings on level 2.  The front-right gap is left open
    // so visitors can step naturally from the final stair onto the upper floor.
    drawHorizontalRailingZ(15.35f,25.15f,32.65f,SECOND_FLOOR_Y);
    drawHorizontalRailingX(15.35f,23.45f,25.15f,SECOND_FLOOR_Y);
    drawHorizontalRailingX(15.35f,19.45f,32.65f,SECOND_FLOOR_Y);
    drawHorizontalRailingX(22.55f,23.45f,32.65f,SECOND_FLOOR_Y);

    // Upper landing details: sign, decorative lamps and two plants.  No benches
    // are placed in the stair circulation zone.
    drawRoomSign("LEVEL 2 - GRAND EXHIBITION HALL",0.0f,16.80f,35.62f,180,10.5f);
    for(float x : {-9.0f,9.0f}) drawWallLamp(x,14.70f,35.55f,180);
    glPushMatrix();
    glTranslatef(0,SECOND_FLOOR_Y,0);
    drawPlant(-13.5f,33.8f,0.62f);
    drawPlant( 12.8f,33.8f,0.62f);
    glPopMatrix();

}

// ================================================================
// Step 10B: Level 2 - ONE large open-plan Grand Exhibition Hall
// ================================================================

void drawUpperLightSculpture() {
    // A large animated centerpiece that is visible from many parts of Level 2.
    drawTexturedBox(0,0.42f,0,5.8f,0.84f,5.8f,texFloor,42);
    drawBox(0,0.90f,0,5.25f,0.12f,5.25f,0.62f,0.46f,0.15f,62,0.70f);
    drawCylinderY(0,0.95f,0,0.32f,2.55f,0.18f,0.20f,0.24f,70,0.82f);

    glPushMatrix();
    glTranslatef(0,3.25f,0);
    glRotatef(centralRotation*0.70f,0,1,0);
    setMaterial(0.18f,0.48f,0.86f,95,0.95f);
    glutSolidTorus(0.16f,1.48f,22,58);
    glRotatef(64.0f,1,0,0);
    setMaterial(0.90f,0.46f,0.16f,95,0.95f);
    glutSolidTorus(0.14f,1.22f,22,58);
    glRotatef(58.0f,0,1,0);
    setMaterial(0.46f,0.78f,0.92f,95,0.95f);
    glutSolidTorus(0.12f,0.94f,22,58);
    glPopMatrix();

    drawGlowSphere(0,3.25f,0,0.30f,0.30f,0.30f,1.0f,0.62f,0.16f,0.72f);
    drawGlowSphere( 1.45f,3.25f,0,0.11f,0.11f,0.11f,0.20f,0.62f,1.0f,0.62f);
    drawGlowSphere(-1.45f,3.25f,0,0.11f,0.11f,0.11f,1.0f,0.40f,0.12f,0.62f);

    drawExhibitPlaque("AURORA ORBIT","KINETIC LIGHT SCULPTURE",0,0.78f,3.95f,180,4.7f);
}

void drawUpperSculpturePod(float x,float z,int style) {
    drawTexturedBox(x,0.46f,z,2.35f,0.92f,2.35f,texFloor,40);
    drawBox(x,0.95f,z,2.05f,0.10f,2.05f,0.60f,0.44f,0.14f,55,0.62f);

    glPushMatrix();
    glTranslatef(x,2.25f,z);
    glRotatef(centralRotation*(0.20f+0.08f*style)+style*28.0f,0,1,0);
    if(style==0) {
        setMaterial(0.72f,0.70f,0.65f,75,0.72f);
        glutSolidIcosahedron();
    } else if(style==1) {
        setMaterial(0.18f,0.50f,0.72f,90,0.88f);
        glutSolidOctahedron();
    } else if(style==2) {
        setMaterial(0.72f,0.36f,0.12f,90,0.86f);
        glRotatef(90,1,0,0);
        glutSolidTorus(0.18f,0.80f,20,46);
    } else {
        setMaterial(0.52f,0.20f,0.62f,88,0.86f);
        glutSolidDodecahedron();
    }
    glPopMatrix();
}

void drawUpperSuspendedMobile(float x,float z) {
    // Hanging art makes the tall second-floor ceiling feel intentional.
    drawCylinderY(x,6.55f,z,0.035f,2.25f,0.18f,0.16f,0.12f,48,0.42f);
    glPushMatrix();
    glTranslatef(x,6.10f,z);
    glRotatef(-centralRotation*0.32f,0,1,0);
    for(int i=0;i<5;i++) {
        float a=2.0f*PI*float(i)/5.0f;
        float px=1.45f*std::cos(a), pz=1.45f*std::sin(a);
        drawCylinderBetween(Vec3(0,0,0),Vec3(px,-0.35f-0.18f*(i%2),pz),0.025f,
                            0.42f,0.34f,0.18f);
        drawGlowSphere(px,-0.48f-0.18f*(i%2),pz,0.20f,0.20f,0.20f,
                       (i%2)?0.20f:0.95f,0.56f,(i%2)?1.0f:0.18f,0.48f);
    }
    glPopMatrix();
}


// Step 12: two additional large feature pieces on the central red promenade.
// They deliberately reuse primitives/materials so no external model dependency is added.
void drawUpperCelestialGlobe(float z) {
    const float x=0.0f;
    drawTexturedBox(x,0.42f,z,5.2f,0.84f,5.2f,texFloor,42);
    drawBox(x,0.90f,z,4.7f,0.12f,4.7f,0.62f,0.46f,0.15f,62,0.70f);
    drawCylinderY(x,0.95f,z,0.34f,2.15f,0.19f,0.18f,0.16f,55,0.42f);

    // Large blue globe with two bronze orbital rings.
    drawScaledSphere(x,3.15f,z,1.48f,1.48f,1.48f,
                     0.065f,0.26f,0.52f,42,0.32f);
    glPushMatrix();
    glTranslatef(x,3.15f,z);
    setMaterial(0.78f,0.48f,0.10f,72,0.70f);
    glutSolidTorus(0.095f,1.78f,20,54);
    glRotatef(63.0f,1,0,0);
    glutSolidTorus(0.075f,1.62f,18,52);
    glPopMatrix();
    drawGlowSphere(x,3.15f,z,0.18f,0.18f,0.18f,1.0f,0.69f,0.14f,0.66f);
    drawExhibitPlaque("CELESTIAL GLOBE","LIGHT & MOTION COLLECTION",
                      x,0.82f,z+3.55f,180,4.9f);
}

void drawUpperCrystalBloom(float z) {
    const float x=0.0f;
    drawTexturedBox(x,0.42f,z,5.2f,0.84f,5.2f,texFloor,42);
    drawBox(x,0.90f,z,4.7f,0.12f,4.7f,0.62f,0.46f,0.15f,62,0.70f);
    drawCylinderY(x,0.95f,z,0.30f,2.05f,0.18f,0.20f,0.22f,58,0.48f);

    glPushMatrix();
    glTranslatef(x,3.10f,z);
    glRotatef(-centralRotation*0.28f,0,1,0);
    for(int i=0;i<8;i++) {
        float a=45.0f*float(i);
        glPushMatrix();
        glRotatef(a,0,0,1);
        glTranslatef(0,0.95f,0);
        drawScaledSphere(0,0,0,0.36f,1.05f,0.30f,
                         (i%2)?0.66f:0.20f,
                         (i%2)?0.24f:0.55f,
                         (i%2)?0.78f:0.88f,58,0.50f);
        glPopMatrix();
    }
    drawGlowSphere(0,0,0,0.50f,0.50f,0.50f,1.0f,0.48f,0.10f,0.72f);
    glPopMatrix();
    drawExhibitPlaque("CRYSTAL BLOOM","CONTEMPORARY LIGHT ART",
                      x,0.82f,z+3.55f,180,4.8f);
}

void drawUpperCommonHall() {
    glPushMatrix();
    // Reuse all ground-relative museum helpers by translating the local floor
    // origin to the real second-floor height.
    glTranslatef(0,SECOND_FLOOR_Y,0);

    // ---------- Grand identity wall / arrival zone ----------
    drawCollectionIntroPanel("GRAND EXHIBITION HALL","ART  LIGHT  PORTRAIT  HERITAGE  SCULPTURE",
                             -5.5f,5.15f,35.58f,180,11.8f);
    drawWallArtwork(-17.4f,4.95f,35.58f,3.0f,3.55f,texPortrait1,180,
                    "PORTRAIT OF IDEAS","LEVEL 2 COLLECTION",true);
    drawWallArtwork(-12.7f,4.95f,35.58f,3.0f,3.55f,texPortrait2,180,
                    "PORTRAIT OF TIME","LEVEL 2 COLLECTION",true);

    // ---------- Left wall: portrait / fine-art promenade ----------
    drawWallArtwork(-23.64f,5.10f,27.0f,3.5f,3.1f,texPortrait3,90,
                    "HUMAN STUDY","PORTRAIT COLLECTION",true);
    drawWallArtwork(-23.64f,5.10f,18.0f,4.0f,3.0f,texPainting1,90,
                    "GOLDEN LANDSCAPE","MASTERWORKS",true);
    drawWallArtwork(-23.64f,5.10f,8.0f,4.0f,3.0f,texPainting6,90,
                    "CITY LIGHTS","MASTERWORKS",true);
    drawWallArtwork(-23.64f,5.10f,-3.0f,4.0f,3.0f,texPainting5,90,
                    "BLUE HORIZON","MASTERWORKS",true);
    drawWallArtwork(-23.64f,5.10f,-14.0f,4.0f,3.0f,texPainting3,90,
                    "FOREST MEMORY","MASTERWORKS",true);
    drawWallArtwork(-23.64f,5.10f,-26.0f,4.0f,3.0f,texPainting8,90,
                    "WILDLIFE STUDY","MASTERWORKS",true);

    // One wall-side photo bench only; the central floor stays open.
    drawBench(-22.90f,29.0f,-90);

    // ---------- Right wall: cultural / calligraphy promenade ----------
    // Keep the front-right stair exit clear, so the first piece starts farther back.
    drawWallArtwork(23.64f,5.10f,19.0f,4.0f,3.0f,texPainting11,-90,
                    "CALLIGRAPHIC RHYTHM","HERITAGE COLLECTION",true);
    drawWallArtwork(23.64f,5.10f,8.0f,4.0f,3.0f,texPainting12,-90,
                    "GEOMETRIC HERITAGE","HERITAGE COLLECTION",true);
    drawWallArtwork(23.64f,5.10f,-3.0f,4.0f,3.0f,texPainting13,-90,
                    "FLORAL MANUSCRIPT","HERITAGE COLLECTION",true);
    drawWallArtwork(23.64f,5.10f,-14.0f,4.0f,3.0f,texPainting14,-90,
                    "FOLK COLOR","HERITAGE COLLECTION",true);
    drawWallArtwork(23.64f,5.10f,-26.0f,4.0f,3.0f,texPainting9,-90,
                    "ARCHIVE MEMORY","HERITAGE COLLECTION",true);

    // ---------- Rear feature wall ----------
    drawArtworkTriptych(0.0f,5.10f,-35.58f,0,
                        texPainting2,texPainting4,texPainting10,
                        "COLOR FIELD","FORM IN MOTION","MONOCHROME",
                        "FEATURE WALL");

    // ---------- Open-floor sculpture zones ----------
    // Three large feature sculptures now occupy the central red promenade:
    // one existing centerpiece plus two additional major works requested for Level 2.
    drawUpperCelestialGlobe(18.0f);
    drawUpperLightSculpture();
    drawUpperCrystalBloom(-18.0f);

    // Four smaller pods create variety without building any partition walls.
    drawUpperSculpturePod(-11.0f,14.0f,0);
    drawUpperSculpturePod( 11.0f,14.0f,1);
    drawUpperSculpturePod(-11.0f,-15.0f,2);
    drawUpperSculpturePod( 11.0f,-15.0f,3);
    drawArtworkLabel("STONE GEOMETRY",-11.0f,0.78f,15.55f,180);
    drawArtworkLabel("BLUE CRYSTAL", 11.0f,0.78f,15.55f,180);
    drawArtworkLabel("BRONZE ORBIT",-11.0f,0.78f,-13.45f,180);
    drawArtworkLabel("VIOLET FORM", 11.0f,0.78f,-13.45f,180);

    // Two classic objects make the upper hall feel like a mixed museum show.
    drawBust(-16.0f,-29.0f,0.82f,0.78f);
    drawVase(16.0f,-29.0f,0.70f,0.36f,0.56f,0.72f);
    drawArtworkLabel("CLASSICAL STUDY",-16.0f,0.68f,-27.55f,180);
    drawArtworkLabel("CERAMIC BLUE",16.0f,0.68f,-27.55f,180);

    // ---------- Ceiling / atmosphere ----------
    drawChandelier(-8.0f,21.0f,0.92f);
    drawChandelier( 8.0f,-21.0f,0.92f);
    drawUpperSuspendedMobile(0.0f,-22.0f);

    // Visible recessed-light accents and decorative greenery near corners.
    for(float z : {24.0f,8.0f,-8.0f,-24.0f}) {
        drawGlowSphere(-8.0f,8.55f,z,0.18f,0.10f,0.18f,1.0f,0.74f,0.34f,0.36f);
        drawGlowSphere( 8.0f,8.55f,z,0.18f,0.10f,0.18f,0.64f,0.78f,1.0f,0.30f);
    }
    drawPlant(-20.5f,-31.0f,0.62f);
    drawPlant( 20.5f,-31.0f,0.62f);

    // The two former floor rosettes are now occupied by the large feature pieces,
    // so no decorative geometry is drawn underneath them.

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
    drawColumn(-8.0f,30.5f); drawColumn(8.0f,30.5f);
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
    // Curated wall-art system
    // Each room now uses themed artwork, consistent eye-level spacing,
    // information labels, picture lights and an introductory curator panel.
    // ============================================================

    // ---------------- Lobby collection ----------------
    drawWallArtwork(-15.5f,5.25f,35.62f,4.5f,3.2f,texPainting6,180,
                    "CITY AFTER DARK","WELCOME COLLECTION",true);
    drawWallArtwork( 15.5f,5.25f,35.62f,4.5f,3.2f,texPainting5,180,
                    "BLUE HORIZON","WELCOME COLLECTION",true);
    drawWallArtwork( 8.7f,5.15f,35.62f,2.25f,2.85f,texPortrait3,180,
                    "PORTRAIT STUDY III","WELCOME PORTRAIT",true);
    drawWallArtwork(-23.64f,5.05f,28.0f,4.0f,3.1f,texPainting1,90,
                    "SUNSET VALLEYS","LANDSCAPE COLLECTION",true);
    drawWallClock(0,5.35f,36.58f,180);
    // Step 7: remove the old large WELCOME curator panel completely.
    // From oblique lobby views its back face looked like a floating white wall.
    // The entrance is cleaner and more open without it.

    // Room signs over corridor entrances
    drawRoomSign("WILDLIFE",-5.70f,7.95f,16.0f,90,3.9f);
    drawRoomSign("CLASSICAL",5.70f,7.95f,16.0f,-90,4.2f);
    drawRoomSign("MODERN ART",-5.70f,7.95f,-1.0f,90,4.5f);
    drawRoomSign("NATURE",5.70f,7.95f,-1.0f,-90,3.8f);
    drawRoomSign("CULTURAL ART",-5.70f,7.95f,-17.0f,90,5.1f);
    drawRoomSign("HISTORY",5.70f,7.95f,-17.0f,-90,4.1f);
    drawRoomSign("SCULPTURE HALL",0,7.95f,-35.60f,0,6.6f);

    // ---------------- Main corridor ----------------
    // Smaller works are deliberately spaced between entrances so the visitor
    // can stop in front of each piece without the walls looking overcrowded.
    drawWallArtwork(-5.70f,4.75f,22.0f,2.8f,2.25f,texPainting10,90,
                    "MUSEUM STUDY","CORRIDOR SERIES",true);
    drawWallArtwork( 5.70f,4.75f,22.0f,2.8f,2.25f,texPainting3,-90,
                    "FOREST SILENCE","CORRIDOR SERIES",true);
    drawWallArtwork(-5.70f,4.75f,7.5f,2.9f,2.30f,texPainting4,90,
                    "GEOMETRY IN MOTION","MODERN COLLECTION",true);
    drawWallArtwork( 5.70f,4.75f,7.5f,2.9f,2.30f,texPainting1,-90,
                    "MOUNTAIN LIGHT","LANDSCAPE COLLECTION",true);
    drawWallArtwork(-5.70f,4.75f,-9.0f,2.9f,2.30f,texPainting5,90,
                    "OPEN WATER","NATURE COLLECTION",true);
    drawWallArtwork( 5.70f,4.75f,-9.0f,2.9f,2.30f,texPainting9,-90,
                    "DESERT HERITAGE","HISTORY COLLECTION",true);
    drawWallArtwork(-5.70f,4.75f,-22.0f,2.8f,2.25f,texPainting11,90,
                    "CALLIGRAPHIC RHYTHM","CULTURAL COLLECTION",true);
    drawWallArtwork( 5.70f,4.75f,-22.0f,2.8f,2.25f,texPainting12,-90,
                    "GEOMETRIC HERITAGE","CULTURAL COLLECTION",true);

    // Corridor wall sconces supplement the picture lights.
    for(float z: {22.0f,7.5f,-9.0f,-22.0f}){
        drawWallLamp(-5.68f,7.35f,z,90);
        drawWallLamp( 5.68f,7.35f,z,-90);
    }

    // ---------------- Wildlife Gallery L1 ----------------
    drawWallArtwork(-23.64f,5.15f,12.0f,4.1f,3.0f,texPainting8,90,
                    "BENGAL TIGER PORTRAIT","WILDLIFE COLLECTION",true);
    drawWallArtwork(-23.64f,5.15f,20.2f,4.1f,3.0f,texPainting3,90,
                    "FOREST CANOPY","WILDLIFE COLLECTION",true);
    drawCollectionIntroPanel("WILDLIFE OF BENGAL","FORM  HABITAT  CONSERVATION",
                             -10.2f,5.15f,8.36f,0,6.1f);
    drawWallArtwork(-18.4f,5.15f,8.36f,4.1f,3.0f,texPainting7,0,
                    "BOTANICAL HABITAT","WILDLIFE COLLECTION",true);
    drawTigerWallRelief(-15.0f,5.15f,23.72f,180);
    drawArtworkLabel("TIGER RELIEF",-15.0f,3.15f,23.58f,180);

    // ---------------- Classical Gallery R1 ----------------
    drawWallArtwork(23.64f,5.15f,12.0f,4.1f,3.0f,texPainting1,-90,
                    "GOLDEN VALLEY","CLASSICAL COLLECTION",true);
    drawWallArtwork(23.64f,5.15f,20.2f,4.1f,3.0f,texPainting10,-90,
                    "MONOCHROME STUDY","CLASSICAL COLLECTION",true);
    drawCollectionIntroPanel("CLASSICAL FORMS","PORTRAIT  SCULPTURE  PROPORTION",
                             10.2f,5.15f,8.36f,0,6.1f);
    drawWallArtwork(18.4f,5.15f,8.36f,4.1f,3.0f,texPainting9,0,
                    "HERITAGE ARCHIVE","CLASSICAL COLLECTION",true);

    // Step 7: portrait-ready frames on the corridor-side wall. These generic
    // portrait textures can later be replaced by the user's/famous-person photos.
    drawWallArtwork(6.36f,5.00f,21.0f,2.35f,2.95f,texPortrait1,90,
                    "PORTRAIT STUDY I","PORTRAIT COLLECTION",true);
    drawWallArtwork(6.36f,5.00f,11.0f,2.35f,2.95f,texPortrait2,90,
                    "PORTRAIT STUDY II","PORTRAIT COLLECTION",true);


    // ---------------- Modern Art Gallery L2 ----------------
    drawWallArtwork(-23.64f,5.15f,-5.8f,4.3f,3.1f,texPainting2,90,
                    "RHYTHM IN BLUE","MODERN COLLECTION",true);
    drawWallArtwork(-23.64f,5.15f,3.7f,4.3f,3.1f,texPainting4,90,
                    "PRIMARY GEOMETRY","MODERN COLLECTION",true);
    drawCollectionIntroPanel("MODERN ART","COLOR  MOTION  EXPERIMENT",
                             -10.4f,5.15f,7.64f,180,5.8f);
    drawWallArtwork(-18.7f,5.15f,7.64f,4.0f,2.9f,texPainting14,180,
                    "FOLK COLOR STUDY","MODERN COLLECTION",true);
    drawWallArtwork(-15.0f,5.15f,-9.64f,4.5f,3.15f,texPainting10,0,
                    "RED CIRCLE STUDY","MODERN COLLECTION",true);

    // ---------------- Nature Gallery R2 ----------------
    drawWallArtwork(23.64f,5.15f,-5.8f,4.3f,3.1f,texPainting5,-90,
                    "BLUE HORIZON","NATURE COLLECTION",true);
    drawWallArtwork(23.64f,5.15f,3.7f,4.3f,3.1f,texPainting3,-90,
                    "FOREST LIGHT","NATURE COLLECTION",true);
    drawCollectionIntroPanel("NATURE & ECOLOGY","LAND  WATER  PLANT LIFE",
                             10.4f,5.15f,7.64f,180,5.8f);
    drawWallArtwork(18.7f,5.15f,7.64f,4.0f,2.9f,texPainting7,180,
                    "BOTANICAL FORM","NATURE COLLECTION",true);
    drawWallArtwork(15.0f,5.15f,-9.64f,4.5f,3.15f,texPainting8,0,
                    "WILDLIFE STUDY","NATURE COLLECTION",true);

    // ---------------- Cultural Art & Calligraphy Gallery L3 ----------------
    // This room answers the 'art gallery' part of the project more clearly:
    // calligraphy-inspired work, geometric heritage, floral manuscript design
    // and folk-art color are displayed as a coherent cultural collection.
    drawWallArtwork(-23.64f,5.10f,-13.1f,3.5f,2.75f,texPainting11,90,
                    "CALLIGRAPHIC RHYTHM","CULTURAL COLLECTION",true);
    drawWallArtwork(-23.64f,5.10f,-17.2f,3.5f,2.75f,texPainting12,90,
                    "GEOMETRIC HERITAGE","CULTURAL COLLECTION",true);
    drawWallArtwork(-23.64f,5.10f,-21.3f,3.5f,2.75f,texPainting13,90,
                    "FLORAL MANUSCRIPT","CULTURAL COLLECTION",true);
    drawCollectionIntroPanel("CALLIGRAPHY & CULTURE","SCRIPT  PATTERN  HERITAGE",
                             -10.4f,5.15f,-10.36f,180,6.2f);
    drawWallArtwork(-18.5f,5.10f,-10.36f,4.0f,3.0f,texPainting14,180,
                    "FOLK COLOR","CULTURAL COLLECTION",true);
    drawWallArtwork(-15.0f,5.10f,-23.72f,4.4f,3.1f,texPainting13,0,
                    "MANUSCRIPT GARDEN","CULTURAL COLLECTION",true);
    // One rest bench placed against the outer wall, not in the center of the gallery.
    drawBench(-22.90f,-17.0f,-90);

    // ---------------- History & Artifacts Gallery R3 ----------------
    drawWallArtwork(23.64f,5.10f,-13.2f,3.9f,2.9f,texPainting9,-90,
                    "DESERT HERITAGE","HISTORY COLLECTION",true);
    drawWallArtwork(23.64f,5.10f,-21.0f,3.9f,2.9f,texPainting10,-90,
                    "ARCHIVE POSTER","HISTORY COLLECTION",true);
    drawCollectionIntroPanel("HISTORY & ARTIFACTS","OBJECTS  MEMORY  MATERIAL",
                             10.4f,5.15f,-10.36f,180,6.1f);
    drawWallMask(16.1f,5.15f,-10.36f,180,0.48f,0.28f,0.14f);
    drawWallMask(20.1f,5.15f,-10.36f,180,0.34f,0.22f,0.12f);
    drawArtworkLabel("HERITAGE MASKS",18.1f,3.15f,-10.16f,180);

    // ---------------- Back Sculpture Hall ----------------
    drawWallArtwork(-17.0f,5.25f,-35.58f,4.4f,3.1f,texPainting9,0,
                    "MATERIAL MEMORY","SCULPTURE HALL",true);
    drawWallArtwork( 17.0f,5.25f,-35.58f,4.4f,3.1f,texPainting10,0,
                    "FORM & SHADOW","SCULPTURE HALL",true);
    drawWallArtwork(-23.64f,5.25f,-29.2f,4.1f,3.0f,texPainting2,90,
                    "BLUE MOTION","SCULPTURE HALL",true);
    drawWallArtwork( 23.64f,5.25f,-29.2f,4.1f,3.0f,texPainting3,-90,
                    "FOREST MEMORY","SCULPTURE HALL",true);
    drawCollectionIntroPanel("SCULPTURE HALL","FORM  SPACE  MATERIAL",
                             0,5.05f,-35.60f,0,6.4f);
    drawChandelier(0,-30.0f,1.0f);
    // Keep one wall-side bench so the sculpture hall stays open and walkable.
    drawBench(-22.90f,-30.0f,-90);

    // Step 5 visitor-experience polish: a small photo-friendly feature wall.
    // It sits against the lobby side wall, leaves the circulation path open,
    // and gives a natural place for visitors to sit/stand for a museum photo.
    // Step 10A: the old Memory Corner occupied the new staircase footprint.
    // Keep the visitor artwork on the opposite wall instead of overlapping stairs.
    drawWallArtwork(-23.64f,5.05f,32.0f,3.55f,2.85f,texPainting6,90,
                    "MUSEUM MOMENT","VISITOR COLLECTION",true);
    drawPlant(-20.8f,24.9f,0.58f);

    // Two compact directional boards make the larger multi-room museum easier
    // to navigate without adding more furniture in the center.
    drawRoomSign("GALLERIES  <",-5.70f,3.15f,27.8f,90,3.4f);
    drawRoomSign("GALLERIES  >", 5.70f,3.15f,27.8f,-90,3.4f);

    // Subtle low-level wayfinding lights add depth without adding furniture.
    drawLowGuideLights();

    // Ceiling fans in long circulation areas
    drawCeilingFan(0,16.0f,0.9f);
    drawCeilingFan(0,-5.0f,0.9f);
    drawCeilingFan(0,-19.0f,0.9f);
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
// Animal Art: Natural-color wildlife exhibits
// ================================================================

void drawTigerStripe(float x,float y,float z,float sx,float sy,float sz,float rotZ=0.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotZ,0,0,1);
    glScalef(sx,sy,sz);
    setMaterial(0.035f,0.028f,0.022f,18,0.08f);
    glutSolidSphere(1.0,18,12);
    glPopMatrix();
}

void drawTigerModel() {
    // Step 9: more animal-like Royal Bengal Tiger built only from OpenGL
    // primitives.  Stronger orange/black/white colors and lower specular
    // reflection keep the coat from washing out under gallery lights.
    const float OR=0.93f, OG=0.39f, OB=0.06f;
    const float OR2=0.80f, OG2=0.27f, OB2=0.045f;
    const float CR=0.98f, CG=0.90f, CB=0.74f;
    const float BK=0.018f;

    // Long feline torso with distinct shoulder and haunch masses.
    drawScaledSphere(-0.42f,1.48f,0,2.35f,0.67f,0.56f,OR,OG,OB,14,0.025f);
    drawScaledSphere( 0.98f,1.56f,0,0.88f,0.72f,0.58f,OR,OG,OB,14,0.025f);
    drawScaledSphere(-1.62f,1.46f,0,0.92f,0.74f,0.58f,OR2,OG2,OB2,14,0.025f);

    // Belly and chest patches.
    drawScaledSphere(-0.18f,1.02f,0,1.58f,0.27f,0.53f,CR,CG,CB,12,0.03f);
    drawScaledSphere( 1.23f,1.42f,0,0.50f,0.58f,0.50f,CR,CG,CB,12,0.03f);

    // Neck slopes naturally into the head.
    drawCylinderBetween(Vec3(1.10f,1.72f,0),Vec3(1.72f,2.10f,0),0.48f,OR,OG,OB);
    drawScaledSphere(2.00f,2.18f,0,0.73f,0.62f,0.54f,OR,OG,OB,14,0.025f);

    // Broad cheeks + forward muzzle.
    drawScaledSphere(2.45f,2.12f, 0.26f,0.52f,0.38f,0.31f,OR,OG,OB,14,0.04f);
    drawScaledSphere(2.45f,2.12f,-0.26f,0.52f,0.38f,0.31f,OR,OG,OB,14,0.04f);
    drawScaledSphere(2.77f,1.98f, 0.24f,0.42f,0.27f,0.25f,CR,CG,CB,10,0.02f);
    drawScaledSphere(2.77f,1.98f,-0.24f,0.42f,0.27f,0.25f,CR,CG,CB,10,0.02f);
    drawScaledSphere(2.72f,1.79f,0,0.42f,0.18f,0.28f,CR,CG,CB,10,0.02f);

    // Nose and mouth line.
    drawScaledSphere(3.10f,2.01f,0,0.17f,0.11f,0.18f,BK,BK,BK,8,0.02f);
    drawCylinderBetween(Vec3(2.98f,1.86f,-0.20f),Vec3(2.98f,1.86f,0.20f),0.018f,0.08f,0.035f,0.028f);

    // Eyes with amber iris and black pupil.
    for(int side=-1; side<=1; side+=2){
        float z=side*0.39f;
        drawSphere(2.34f,2.42f,z,0.105f,0.93f,0.66f,0.16f,18,0.05f);
        drawSphere(2.42f,2.42f,z,0.052f,0.015f,0.012f,0.008f,8,0.02f);
        drawSphere(2.45f,2.46f,z+side*0.01f,0.018f,0.95f,0.94f,0.82f,4,0.0f);
    }

    // Distinct upright tiger ears: dark outer triangles + warm inner ears.
    for(int side=-1; side<=1; side+=2){
        float z=side*0.43f;
        glPushMatrix();
        glTranslatef(1.73f,2.60f,z);
        glRotatef(side*8.0f,1,0,0);
        drawConeY(0,0,0,0.29f,0.62f,0.055f,0.024f,0.012f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.77f,2.64f,z+side*0.018f);
        glRotatef(side*8.0f,1,0,0);
        drawConeY(0,0,0,0.17f,0.41f,0.88f,0.44f,0.30f);
        glPopMatrix();
    }

    // Four smoother feline legs: thighs, joints, lower legs and broad paws.
    struct Leg { Vec3 hip,knee,ankle,paw; };
    Leg legs[4] = {
        {Vec3(-1.35f,1.42f, 0.43f),Vec3(-1.45f,0.90f, 0.44f),Vec3(-1.50f,0.31f, 0.44f),Vec3(-1.33f,0.14f, 0.45f)},
        {Vec3(-0.74f,1.40f,-0.43f),Vec3(-0.79f,0.87f,-0.44f),Vec3(-0.84f,0.30f,-0.44f),Vec3(-0.68f,0.14f,-0.45f)},
        {Vec3( 0.72f,1.45f, 0.44f),Vec3( 0.76f,0.88f, 0.44f),Vec3( 0.70f,0.29f, 0.45f),Vec3( 0.88f,0.14f, 0.46f)},
        {Vec3( 1.30f,1.44f,-0.44f),Vec3( 1.30f,0.87f,-0.45f),Vec3( 1.24f,0.29f,-0.45f),Vec3( 1.42f,0.14f,-0.46f)}
    };
    for(int i=0;i<4;i++){
        drawScaledSphere(legs[i].hip.x,legs[i].hip.y,legs[i].hip.z,0.39f,0.48f,0.34f,OR2,OG2,OB2,12,0.03f);
        drawCylinderBetween(legs[i].hip,legs[i].knee,0.20f,OR2,OG2,OB2);
        drawScaledSphere(legs[i].knee.x,legs[i].knee.y,legs[i].knee.z,0.22f,0.22f,0.20f,OR,OG,OB,10,0.02f);
        drawCylinderBetween(legs[i].knee,legs[i].ankle,0.16f,OR,OG,OB);
        drawScaledSphere(legs[i].paw.x,legs[i].paw.y,legs[i].paw.z,0.40f,0.16f,0.29f,OR2,OG2,OB2,10,0.02f);
        float side=(legs[i].paw.z>0)?1.0f:-1.0f;
        for(int t=-1;t<=1;t++)
            drawScaledSphere(legs[i].paw.x+0.08f*t,0.10f,legs[i].paw.z+side*0.26f,0.025f,0.032f,0.020f,BK,BK,BK,4,0.0f);
    }

    // Long curved tail with black rings/tip.
    Vec3 tail[8]={
        Vec3(-2.18f,1.58f,0.02f),Vec3(-2.68f,1.52f,0.08f),Vec3(-3.04f,1.67f,0.16f),
        Vec3(-3.30f,1.95f,0.23f),Vec3(-3.38f,2.28f,0.28f),Vec3(-3.27f,2.59f,0.30f),
        Vec3(-3.02f,2.82f,0.29f),Vec3(-2.77f,2.91f,0.27f)};
    for(int i=0;i<7;i++){
        bool blackSeg=(i==2 || i==4 || i==6);
        drawCylinderBetween(tail[i],tail[i+1],0.12f, blackSeg?BK:OR2, blackSeg?BK:OG2, blackSeg?BK:OB2);
    }

    // Bengal stripes: strong black bands down both body sides.
    const float sx[8]={-1.68f,-1.30f,-0.92f,-0.52f,-0.10f,0.30f,0.67f,1.00f};
    const float sy[8]={ 1.93f, 2.00f, 2.05f, 2.08f, 2.09f,2.05f,1.98f,1.88f};
    for(int i=0;i<8;i++){
        float lean=(i%2==0)?14.0f:-14.0f;
        float len=0.40f+0.05f*(i%3);
        drawTigerStripe(sx[i],sy[i], 0.61f,0.060f,len,0.035f,lean);
        drawTigerStripe(sx[i],sy[i],-0.61f,0.060f,len,0.035f,-lean);
    }

    // Neck and face stripes.
    for(int side=-1;side<=1;side+=2){
        float z=side*0.50f;
        drawTigerStripe(1.38f,2.12f,z,0.060f,0.40f,0.033f,side*24.0f);
        drawTigerStripe(1.74f,2.37f,z,0.052f,0.30f,0.030f,side*30.0f);
        drawTigerStripe(2.06f,2.55f,z,0.050f,0.23f,0.026f,side*22.0f);
        drawTigerStripe(2.39f,2.50f,z,0.045f,0.20f,0.024f,side*38.0f);
    }

    // Leg stripes.
    for(int i=0;i<4;i++){
        float side=(legs[i].paw.z>0)?1.0f:-1.0f;
        drawTigerStripe(legs[i].knee.x,0.88f,legs[i].knee.z+side*0.16f,0.055f,0.16f,0.026f,0);
        drawTigerStripe(legs[i].ankle.x,0.46f,legs[i].ankle.z+side*0.13f,0.050f,0.13f,0.024f,0);
    }

    // White whiskers.
    for(int side=-1;side<=1;side+=2){
        float z=side*0.27f;
        drawCylinderBetween(Vec3(2.94f,2.02f,z),Vec3(3.48f,2.13f,z+side*0.18f),0.010f,0.90f,0.88f,0.80f);
        drawCylinderBetween(Vec3(2.94f,1.94f,z),Vec3(3.44f,1.91f,z+side*0.22f),0.009f,0.90f,0.88f,0.80f);
    }
}

void drawTigerArtwork() {
    glPushMatrix();
    glTranslatef(-15.0f,0.35f,16.0f);
    glTranslatef(artXform[0].tx,artXform[0].ty,artXform[0].tz);
    glRotatef(artXform[0].rotY,0,1,0);
    glScalef(artXform[0].scale,artXform[0].scale,artXform[0].scale);
    drawTexturedBox(0,0.25f,0,8.3f,0.5f,5.2f,texFloor,38);
    glTranslatef(-0.2f,0.45f,0);
    drawTigerModel();
    glPopMatrix();

    drawPlinthFrontLabel("ROYAL BENGAL TIGER",-10.74f,0.60f,16.0f,90,4.55f);
    drawRopeBarrier(-10.6f,13.2f,-10.6f,18.8f);
}

// ================================================================
// Animal Art: Asian Elephant
// ================================================================

void drawElephantModel() {
    // Step 12: deeper natural Asian-elephant gray and clearer head/ear silhouette.
    const float G=0.42f, G2=0.36f, G3=0.48f;
    const float ER=0.60f, EG=0.48f, EB=0.46f;

    // Large barrel body, shoulders and rump.
    drawScaledSphere(-0.32f,1.72f,0,2.30f,1.18f,0.95f,G,G+0.01f,G+0.02f,14,0.05f);
    drawScaledSphere( 0.98f,1.82f,0,1.02f,1.06f,0.93f,G2,G2+0.01f,G2+0.02f,14,0.05f);
    drawScaledSphere(-1.63f,1.66f,0,0.95f,1.00f,0.88f,G,G+0.01f,G+0.02f,14,0.05f);

    // Neck and head with a clear Asian-elephant forehead dome.
    drawCylinderBetween(Vec3(1.02f,1.82f,0),Vec3(1.55f,2.05f,0),0.68f,G2,G2+0.01f,G2+0.02f);
    drawScaledSphere(1.88f,2.13f,0,1.05f,0.96f,0.86f,G,G+0.01f,G+0.02f,12,0.025f);
    drawScaledSphere(2.10f,2.60f,0,0.66f,0.52f,0.70f,G3,G3,G3+0.01f,12,0.04f);

    // Asian elephant ears are broad but smaller than African elephant ears.
    for(int side=-1;side<=1;side+=2){
        float z=side*0.78f;
        drawScaledSphere(1.38f,2.15f,z,0.76f,0.90f,0.11f,G2,G2+0.01f,G2+0.02f,10,0.02f);
        drawScaledSphere(1.45f,2.13f,z+side*0.045f,0.49f,0.61f,0.055f,ER,EG,EB,8,0.02f);
    }

    // Curved trunk, thick at base and tapering towards the tip.
    Vec3 t[7]={Vec3(2.43f,2.00f,0),Vec3(2.73f,1.63f,0),Vec3(2.87f,1.23f,0),
               Vec3(2.88f,0.84f,0),Vec3(2.96f,0.55f,0),Vec3(3.17f,0.40f,0),Vec3(3.42f,0.48f,0)};
    float tr[6]={0.26f,0.23f,0.20f,0.17f,0.14f,0.11f};
    for(int i=0;i<6;i++) drawCylinderBetween(t[i],t[i+1],tr[i],G2,G2+0.01f,G2+0.02f);
    drawScaledSphere(3.44f,0.49f,0,0.15f,0.12f,0.13f,G2,G2+0.01f,G2+0.02f,8,0.02f);

    // Four thick legs with slightly tapered lower portions and broad feet.
    struct ELeg { Vec3 hip,knee,ankle; };
    ELeg legs[4]={
        {Vec3(-1.28f,1.42f, 0.64f),Vec3(-1.29f,0.84f, 0.64f),Vec3(-1.30f,0.20f, 0.64f)},
        {Vec3(-0.64f,1.40f,-0.64f),Vec3(-0.65f,0.83f,-0.64f),Vec3(-0.66f,0.20f,-0.64f)},
        {Vec3( 0.62f,1.46f, 0.64f),Vec3( 0.64f,0.85f, 0.64f),Vec3( 0.65f,0.20f, 0.64f)},
        {Vec3( 1.14f,1.44f,-0.64f),Vec3( 1.15f,0.84f,-0.64f),Vec3( 1.16f,0.20f,-0.64f)}
    };
    for(int i=0;i<4;i++){
        drawCylinderBetween(legs[i].hip,legs[i].knee,0.30f,G,G+0.01f,G+0.02f);
        drawCylinderBetween(legs[i].knee,legs[i].ankle,0.25f,G2,G2+0.01f,G2+0.02f);
        drawScaledSphere(legs[i].ankle.x,0.19f,legs[i].ankle.z,0.42f,0.20f,0.40f,0.31f,0.32f,0.34f,10,0.03f);
        float front=(legs[i].ankle.z>0)?1.0f:-1.0f;
        for(int toe=-1;toe<=1;toe++)
            drawScaledSphere(legs[i].ankle.x+toe*0.11f,0.21f,legs[i].ankle.z+front*0.36f,
                             0.052f,0.040f,0.023f,0.79f,0.74f,0.61f,8,0.02f);
    }

    // Tusks.
    glPushMatrix(); glTranslatef(2.40f,1.78f, 0.38f); glRotatef(70,0,1,0); glRotatef(-35,1,0,0); setMaterial(0.91f,0.86f,0.68f,20,0.08f); glutSolidCone(0.105f,0.78f,18,8); glPopMatrix();
    glPushMatrix(); glTranslatef(2.40f,1.78f,-0.38f); glRotatef(110,0,1,0); glRotatef(-35,1,0,0); setMaterial(0.91f,0.86f,0.68f,20,0.08f); glutSolidCone(0.105f,0.78f,18,8); glPopMatrix();

    // Eyes and tiny catchlights.
    for(int side=-1;side<=1;side+=2){
        float z=side*0.55f;
        drawSphere(2.33f,2.39f,z,0.070f,0.035f,0.028f,0.020f,10,0.02f);
        drawSphere(2.36f,2.42f,z+side*0.008f,0.018f,0.88f,0.84f,0.72f,4,0.0f);
    }

    // Subtle forehead/skin creases.
    drawCylinderBetween(Vec3(2.15f,2.64f, 0.28f),Vec3(2.35f,2.47f, 0.34f),0.018f,0.20f,0.21f,0.22f);
    drawCylinderBetween(Vec3(2.15f,2.64f,-0.28f),Vec3(2.35f,2.47f,-0.34f),0.018f,0.20f,0.21f,0.22f);

    // Tail and dark tuft.
    Vec3 ta(-2.45f,1.54f,0), tb(-2.72f,0.93f,0.05f), tc(-2.78f,0.48f,0.08f);
    drawCylinderBetween(ta,tb,0.070f,G2,G2+0.01f,G2+0.02f);
    drawCylinderBetween(tb,tc,0.050f,G2,G2+0.01f,G2+0.02f);
    drawScaledSphere(-2.79f,0.38f,0.08f,0.11f,0.24f,0.11f,0.08f,0.065f,0.052f,8,0.02f);
}

void drawElephantArtwork() {
    glPushMatrix();
    glTranslatef(15.0f,0.35f,-1.0f);
    glTranslatef(artXform[1].tx,artXform[1].ty,artXform[1].tz);
    glRotatef(180+artXform[1].rotY,0,1,0);
    glScalef(artXform[1].scale,artXform[1].scale,artXform[1].scale);
    drawTexturedBox(0,0.25f,0,8.3f,0.5f,5.2f,texFloor,38);
    glTranslatef(-0.2f,0.45f,0);
    drawElephantModel();
    glPopMatrix();

    drawPlinthFrontLabel("ASIAN ELEPHANT",10.74f,0.60f,-1.0f,-90,4.30f);
    drawRopeBarrier(10.6f,-3.8f,10.6f,1.8f);
}

// ================================================================
// Animal Art: Spotted Deer
// ================================================================

void drawDeerModel() {
    // Step 12: spotted deer/chital with stronger chestnut coat, slimmer limbs and a
    // narrower face so the silhouette reads much closer to a real deer.
    const float BR=0.48f, BG=0.22f, BB=0.07f;
    const float BR2=0.62f, BG2=0.30f, BB2=0.10f;
    const float TANR=0.78f, TANG=0.48f, TANB=0.18f;
    const float WHR=0.92f, WHG=0.83f, WHB=0.66f;
    const float HOOF=0.045f;

    // Slender torso, shoulder and rump.
    drawScaledSphere(-0.28f,1.58f,0,1.82f,0.58f,0.45f,BR2,BG2,BB2,12,0.02f);
    drawScaledSphere( 0.86f,1.67f,0,0.72f,0.72f,0.49f,BR,BG,BB,14,0.04f);
    drawScaledSphere(-1.35f,1.58f,0,0.72f,0.70f,0.52f,BR2,BG2,BB2,14,0.04f);

    // Cream belly/chest.
    drawScaledSphere(-0.18f,1.10f,0,1.20f,0.20f,0.43f,TANR,TANG,TANB,10,0.02f);
    drawScaledSphere( 0.94f,1.55f,0,0.32f,0.45f,0.40f,WHR,WHG,WHB,10,0.02f);

    // Long tapered neck.
    drawCylinderBetween(Vec3(0.78f,1.70f,0),Vec3(1.38f,2.72f,0),0.27f,BR,BG,BB);
    drawScaledSphere(1.43f,2.54f,0,0.42f,0.68f,0.35f,BR,BG,BB,12,0.03f);

    // Narrow deer head and muzzle.
    drawScaledSphere(1.70f,2.99f,0,0.44f,0.36f,0.28f,BR2,BG2,BB2,10,0.02f);
    drawScaledSphere(2.08f,2.93f,0,0.42f,0.23f,0.22f,TANR,TANG,TANB,10,0.02f);
    drawScaledSphere(2.38f,2.92f,0,0.13f,0.095f,0.105f,0.035f,0.026f,0.020f,8,0.02f);
    drawScaledSphere(1.82f,2.78f,0,0.29f,0.18f,0.25f,WHR,WHG,WHB,8,0.01f);

    // Large alert ears.
    for(int side=-1;side<=1;side+=2){
        glPushMatrix();
        glTranslatef(1.47f,3.34f,side*0.27f);
        glRotatef(side*10.0f,1,0,0);
        drawConeY(0,0,0,0.18f,0.48f,BR2,BG2,BB2);
        glPopMatrix();
        drawScaledSphere(1.49f,3.42f,side*0.28f,0.085f,0.18f,0.045f,0.76f,0.46f,0.34f,8,0.01f);
    }

    // Eyes with catchlight.
    for(int side=-1;side<=1;side+=2){
        float z=side*0.25f;
        drawSphere(1.99f,3.07f,z,0.060f,0.025f,0.017f,0.010f,10,0.02f);
        drawSphere(2.015f,3.09f,z+side*0.006f,0.014f,0.95f,0.90f,0.76f,4,0.0f);
    }

    // Long thin legs with visible knees/hocks and black hooves.
    struct DLeg { Vec3 hip,knee,ankle,paw; };
    DLeg legs[4]={
        {Vec3(-1.10f,1.40f, 0.28f),Vec3(-1.14f,0.87f, 0.28f),Vec3(-1.25f,0.32f, 0.29f),Vec3(-1.22f,0.08f, 0.29f)},
        {Vec3(-0.56f,1.39f,-0.28f),Vec3(-0.60f,0.85f,-0.28f),Vec3(-0.70f,0.31f,-0.29f),Vec3(-0.68f,0.08f,-0.29f)},
        {Vec3( 0.54f,1.43f, 0.28f),Vec3( 0.62f,0.86f, 0.28f),Vec3( 0.74f,0.31f, 0.29f),Vec3( 0.78f,0.08f, 0.29f)},
        {Vec3( 0.98f,1.42f,-0.28f),Vec3( 1.05f,0.85f,-0.28f),Vec3( 1.15f,0.31f,-0.29f),Vec3( 1.19f,0.08f,-0.29f)}
    };
    for(int i=0;i<4;i++){
        drawCylinderBetween(legs[i].hip,legs[i].knee,0.095f,BR,BG,BB);
        drawScaledSphere(legs[i].knee.x,legs[i].knee.y,legs[i].knee.z,0.12f,0.11f,0.10f,BR,BG,BB,8,0.01f);
        drawCylinderBetween(legs[i].knee,legs[i].ankle,0.060f,0.30f,0.13f,0.045f);
        drawCylinderBetween(legs[i].ankle,legs[i].paw,0.045f,0.23f,0.10f,0.032f);
        drawScaledSphere(legs[i].paw.x,legs[i].paw.y,legs[i].paw.z,0.12f,0.07f,0.10f,HOOF,HOOF*0.8f,HOOF*0.6f,6,0.01f);
    }

    // Branched antlers.
    const float AR=0.31f, AG=0.17f, AB=0.070f;
    Vec3 L0(1.60f,3.24f,0.17f), L1(1.42f,3.82f,0.24f), L2(1.18f,4.33f,0.30f);
    Vec3 R0(1.60f,3.24f,-0.17f),R1(1.42f,3.82f,-0.24f),R2(1.18f,4.33f,-0.30f);
    drawCylinderBetween(L0,L1,0.055f,AR,AG,AB); drawCylinderBetween(L1,L2,0.046f,AR,AG,AB);
    drawCylinderBetween(L1,Vec3(1.76f,4.10f,0.34f),0.038f,AR,AG,AB);
    drawCylinderBetween(L2,Vec3(1.05f,4.66f,0.34f),0.034f,AR,AG,AB);
    drawCylinderBetween(L2,Vec3(1.47f,4.53f,0.40f),0.034f,AR,AG,AB);
    drawCylinderBetween(R0,R1,0.055f,AR,AG,AB); drawCylinderBetween(R1,R2,0.046f,AR,AG,AB);
    drawCylinderBetween(R1,Vec3(1.76f,4.10f,-0.34f),0.038f,AR,AG,AB);
    drawCylinderBetween(R2,Vec3(1.05f,4.66f,-0.34f),0.034f,AR,AG,AB);
    drawCylinderBetween(R2,Vec3(1.47f,4.53f,-0.40f),0.034f,AR,AG,AB);

    // Clear white spots on both sides of the brown coat.
    const float spx[14]={-1.45f,-1.15f,-0.82f,-0.48f,-0.13f,0.23f,0.57f,-1.28f,-0.95f,-0.60f,-0.25f,0.10f,0.44f,0.72f};
    const float spy[14]={ 1.83f, 1.96f, 2.01f, 2.03f, 2.02f,1.98f,1.88f, 1.55f,1.65f,1.70f,1.71f,1.69f,1.62f,1.52f};
    for(int i=0;i<14;i++){
        drawScaledSphere(spx[i],spy[i], 0.49f,0.065f,0.050f,0.026f,0.95f,0.90f,0.77f,6,0.0f);
        drawScaledSphere(spx[i],spy[i],-0.49f,0.065f,0.050f,0.026f,0.95f,0.90f,0.77f,6,0.0f);
    }

    // Short tail with white underside/tip.
    drawCylinderBetween(Vec3(-1.72f,1.74f,0),Vec3(-2.02f,1.97f,0.03f),0.080f,BR2,BG2,BB2);
    drawScaledSphere(-2.07f,2.02f,0.03f,0.13f,0.22f,0.12f,WHR,WHG,WHB,8,0.01f);
}

void drawDeerArtwork() {
    glPushMatrix();
    glTranslatef(0,0.35f,-29.0f);
    glTranslatef(artXform[2].tx,artXform[2].ty,artXform[2].tz);
    glRotatef(-90+artXform[2].rotY,0,1,0);
    glScalef(artXform[2].scale,artXform[2].scale,artXform[2].scale);
    drawTexturedBox(0,0.25f,0,8.2f,0.5f,5.3f,texFloor,38);
    glTranslatef(0,0.4f,0);
    drawDeerModel();
    glPopMatrix();

    drawPlinthFrontLabel("SPOTTED DEER",0,0.60f,-26.20f,0,3.85f);
}

// ================================================================
// Central animated abstract art
// ================================================================

void drawAbstractSculpture() {
    glPushMatrix();
    glTranslatef(-15.0f,0.35f,-1.0f);
    glTranslatef(artXform[3].tx,artXform[3].ty,artXform[3].tz);
    glRotatef(artXform[3].rotY,0,1,0);
    glScalef(artXform[3].scale,artXform[3].scale,artXform[3].scale);

    drawCylinderY(0,0,0,2.1f,0.55f,0.22f,0.22f,0.25f,65,0.8f);
    glTranslatef(0,2.8f,0);
    glRotatef(centralRotation,0,1,0);

    setMaterial(0.82f,0.10f,0.14f,90,0.70f,0.42f);
    glutSolidTorus(0.18f,1.50f,24,52);
    glRotatef(60,1,0,0);
    setMaterial(0.10f,0.42f,0.90f,90,0.70f,0.42f);
    glutSolidTorus(0.16f,1.20f,24,52);
    glRotatef(60,0,0,1);
    setMaterial(0.92f,0.62f,0.08f,90,0.72f,0.42f);
    glutSolidTorus(0.14f,0.92f,24,52);
    drawSphere(0,0,0,0.48f,0.86f,0.88f,0.92f,100,0.78f);
    glPopMatrix();

    drawArtworkLabel("KINETIC SCULPTURE",-10.35f,0.82f,-1.0f,90);
}

// ================================================================
// Display cases and additional art objects
// ================================================================

void drawTransparentPanel(float x,float y,float z,float sx,float sy,float sz,
                          float r=0.58f,float g=0.78f,float b=0.88f,float alpha=0.10f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterialAlpha(r,g,b,alpha,90,0.95f);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawAncientPotteryArtifact() {
    // Warm terracotta vessel: unique history object instead of a repeated teapot.
    drawScaledSphere(0,1.25f,0,0.56f,0.72f,0.56f,0.52f,0.20f,0.08f,32,0.28f);
    drawCylinderY(0,1.67f,0,0.27f,0.40f,0.46f,0.15f,0.06f,30,0.24f);
    drawCylinderY(0,2.04f,0,0.38f,0.10f,0.36f,0.11f,0.045f,28,0.22f);
    // Decorative bands
    drawBox(0,1.35f,0.56f,0.86f,0.08f,0.035f,0.86f,0.62f,0.22f,25,0.18f);
    drawBox(0,1.10f,0.56f,0.72f,0.06f,0.035f,0.86f,0.62f,0.22f,25,0.18f);
}

void drawCrownArtifact() {
    // Stylized ceremonial crown / metalwork exhibit.
    glPushMatrix();
    glTranslatef(0,1.18f,0);
    setMaterial(0.82f,0.61f,0.14f,95,0.95f);
    glutSolidTorus(0.10f,0.48f,18,36);
    for(int i=0;i<8;i++){
        float a=i*2.0f*PI/8.0f;
        float x=0.44f*std::cos(a), z=0.44f*std::sin(a);
        drawConeY(x,0.05f,z,0.11f,0.62f,0.86f,0.64f,0.15f);
        drawSphere(x,0.70f,z,0.085f,0.75f,0.08f,0.10f,80,0.9f);
    }
    drawSphere(0,0.56f,0,0.18f,0.12f,0.34f,0.72f,90,0.95f);
    glPopMatrix();
}

void drawManuscriptArtifact() {
    // Open manuscript/book on a dark archival cradle.
    drawBox(0,0.90f,0,1.55f,0.18f,1.05f,0.18f,0.12f,0.08f,35,0.25f);
    glPushMatrix();
    glTranslatef(-0.36f,1.10f,0);
    glRotatef(-8.0f,0,0,1);
    drawBox(0,0,0,0.72f,0.06f,0.92f,0.82f,0.72f,0.48f,18,0.10f);
    for(int i=0;i<5;i++) drawBox(-0.05f,0.04f,0.28f-i*0.14f,0.48f,0.018f,0.025f,0.22f,0.16f,0.09f,12,0.05f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.36f,1.10f,0);
    glRotatef(8.0f,0,0,1);
    drawBox(0,0,0,0.72f,0.06f,0.92f,0.84f,0.74f,0.50f,18,0.10f);
    for(int i=0;i<5;i++) drawBox(0.05f,0.04f,0.28f-i*0.14f,0.48f,0.018f,0.025f,0.22f,0.16f,0.09f,12,0.05f);
    glPopMatrix();
}

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

void drawAbstractBronze(float x,float z,float scale=1.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glScalef(scale,scale,scale);
    drawTexturedBox(0,0.52f,0,1.8f,1.04f,1.8f,texFloor,38);
    glPushMatrix();
    glTranslatef(0,2.05f,0);
    glRotatef(90,1,0,0);
    setMaterial(0.52f,0.24f,0.08f,88,0.82f);
    glutSolidTorus(0.18f,0.78f,20,42);
    glPopMatrix();
    drawScaledSphere(0,2.05f,0,0.22f,0.95f,0.22f,0.66f,0.35f,0.12f,75,0.72f);
    drawSphere(0,3.02f,0,0.20f,0.74f,0.48f,0.18f,80,0.78f);
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

    // Different artifact in every case.
    if(artifactType==0) drawAncientPotteryArtifact();
    else if(artifactType==1) drawCrownArtifact();
    else drawManuscriptArtifact();

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
    // Classical gallery - Step 8 cleanup:
    // keep the center aisle open and use only two well-spaced busts.
    drawBust(11.2f,19.6f,0.88f,0.78f);
    drawBust(19.0f,14.6f,0.90f,0.70f);
    drawArtworkLabel("CLASSICAL BUST",11.2f,0.62f,18.25f,180);
    drawArtworkLabel("MARBLE STUDY",19.0f,0.62f,13.25f,180);

    // Modern gallery: geometric companion pieces around kinetic sculpture
    for(int k=0;k<3;k++){
        float z=-6.0f+k*5.0f;
        drawTexturedBox(-19.6f,0.50f,z,1.8f,1.0f,1.8f,texFloor,35);
        glPushMatrix();
        glTranslatef(-19.6f,2.0f,z);
        glRotatef(centralRotation*(0.35f+0.1f*k)+k*35.0f,0,1,0);
        if(k==0){ setMaterial(0.20f,0.55f,0.76f,85,0.8f); glutSolidIcosahedron(); }
        if(k==1){ setMaterial(0.72f,0.18f,0.25f,85,0.8f); glutSolidOctahedron(); }
        if(k==2){ setMaterial(0.82f,0.58f,0.12f,85,0.8f); glutSolidTorus(0.18f,0.70f,18,36); }
        glPopMatrix();
    }

    // Nature room: elephant plus plants and one small bird sculpture.
    // Step 7 layout audit: keep the elephant's viewing zone clear.
    drawPlant(22.0f,6.1f,0.62f);
    drawPlant(22.0f,-7.2f,0.62f);
    drawBirdSculpture(20.2f,4.0f,0.82f);
    drawArtworkLabel("FOREST BIRD",20.2f,0.78f,5.05f,180);

    // Cultural-art gallery: three freestanding study panels.
    // These complement the wall collection without making the walls feel crowded.
    const GLuint culturalTex[3]={texPainting11,texPainting12,texPainting13};
    const char* culturalTitle[3]={"CALLIGRAPHY","MOSAIC","FLORAL STUDY"};
    for(int i=0;i<3;i++){
        float z=-13.4f-i*3.6f;
        drawTexturedBox(-10.0f,1.35f,z,0.18f,2.7f,2.6f,texWood,25);
        drawPainting(-9.82f,3.25f,z,2.2f,1.75f,culturalTex[i],90);
        drawArtworkLabel(culturalTitle[i],-9.70f,1.18f,z,90);
    }

    // History & artifacts room: three DIFFERENT exhibits in transparent cases.
    drawGlassCase(12.0f,-14.0f,0,0);
    drawGlassCase(18.0f,-14.0f,1,0);
    drawGlassCase(15.0f,-20.5f,2,0);
    drawArtworkLabel("ANCIENT POTTERY",12.0f,0.62f,-12.72f,180);
    drawArtworkLabel("CEREMONIAL CROWN",18.0f,0.62f,-12.72f,180);
    drawArtworkLabel("ARCHIVE MANUSCRIPT",15.0f,0.62f,-19.22f,180);
    // Keep only one freestanding vessel so pottery does not repeat everywhere.
    drawVase(21.0f,-17.0f,0.68f,0.62f,0.28f,0.16f);

    // Back sculpture hall: several independent sculptures around deer
    drawBust(-15.5f,-30.0f,0.90f,0.74f);
    drawBust( 15.5f,-30.0f,0.90f,0.64f);
    drawTexturedBox(-8.0f,0.58f,-32.0f,2.0f,1.15f,2.0f,texFloor,36);
    glPushMatrix(); glTranslatef(-8.0f,2.25f,-32.0f); glRotatef(centralRotation*0.5f,0,1,0); setMaterial(0.18f,0.42f,0.62f,90,0.9f); glutSolidTorus(0.22f,0.90f,20,42); glPopMatrix();
    // Replace the repeated teapot with a purpose-built bronze abstract sculpture.
    drawAbstractBronze(8.0f,-32.0f,1.0f);
    drawArtworkLabel("BRONZE FORM",8.0f,0.78f,-30.85f,180);

    // Plants in corners of the side galleries
    drawPlant(-21.5f,10.0f,0.72f); drawPlant(-21.5f,22.0f,0.72f);
    drawPlant(-21.5f,-8.0f,0.72f);
    drawPlant(-21.5f,-23.0f,0.72f); drawPlant(22.0f,-23.2f,0.58f);
}


// ================================================================
// Exterior entrance / plaza
// ================================================================

void drawExterior() {
    // Step 12: much deeper forecourt so the opening camera shows the complete
    // two-storey facade instead of starting almost at the entrance.
    drawTexturedBox(0,-0.28f,51.5f,54.0f,0.50f,31.0f,texFloor,38);
    drawTexturedBox(0,0.01f,51.5f,7.0f,0.06f,31.0f,texCarpet,10);

    // Entrance facade columns and canopy
    drawColumn(-8.0f,37.0f); drawColumn(8.0f,37.0f);
    drawColumn(-12.5f,37.0f); drawColumn(12.5f,37.0f);
    drawBox(0,8.95f,37.0f,30.0f,0.55f,2.2f,0.72f,0.70f,0.64f,45,0.5f);
    drawBox(0,9.38f,36.65f,31.5f,0.25f,2.5f,0.54f,0.40f,0.13f,55,0.65f);

    // External museum title
    drawRoomSign("SMART MUSEUM & ART GALLERY",0,7.45f,36.38f,0,10.8f);

    // Garden planters and lamps
    drawPlant(-20.5f,42.0f,1.0f); drawPlant(20.5f,42.0f,1.0f);
    drawPlant(-19.0f,54.0f,0.82f); drawPlant(19.0f,54.0f,0.82f);
    drawPlant(-15.5f,63.0f,0.72f); drawPlant(15.5f,63.0f,0.72f);
    for(float z : {46.0f,57.0f}) for(float x: {-10.0f,10.0f}){
        drawCylinderY(x,0.0f,z,0.10f,3.1f,0.16f,0.16f,0.17f,40,0.5f);
        drawSphere(x,3.15f,z,0.28f,1.0f,0.80f,0.36f,70,0.9f);
    }

    // Decorative entrance sculptures
    drawTexturedBox(-18.0f,0.55f,37.8f,2.2f,1.1f,2.2f,texFloor,36);
    glPushMatrix(); glTranslatef(-18.0f,2.25f,37.8f); glRotatef(25,0,1,0); setMaterial(0.38f,0.46f,0.58f,80,0.8f); glutSolidIcosahedron(); glPopMatrix();
    drawTexturedBox(18.0f,0.55f,37.8f,2.2f,1.1f,2.2f,texFloor,36);
    glPushMatrix(); glTranslatef(18.0f,2.25f,37.8f); glRotatef(-25,0,1,0); setMaterial(0.60f,0.34f,0.18f,80,0.8f); glutSolidOctahedron(); glPopMatrix();
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
            configureCeilingSpot(GL_LIGHT3,-15.0f, 16.0f,0.92f,0.78f,0.56f); // wildlife
            configureCeilingSpot(GL_LIGHT4, 15.0f, 16.0f,0.94f,0.80f,0.62f); // classical
            configureCeilingSpot(GL_LIGHT5,-15.0f, -1.0f,0.68f,0.78f,1.00f); // modern
            configureCeilingSpot(GL_LIGHT6, 15.0f, -1.0f,0.72f,0.94f,0.76f); // nature
            configureCeilingSpot(GL_LIGHT7,  0.0f,-17.0f,0.96f,0.82f,0.62f,64.0f); // cultural/history axis
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


// Subtle selection ring for the four transformable exhibits.  This makes the
// J/L, I/K, U/O, R/T and +/- controls visually obvious during demonstration.
void drawSelectedObjectMarker() {
    const Vec3 base[4] = {
        Vec3(-15.0f,0.10f,16.0f),
        Vec3( 15.0f,0.10f,-1.0f),
        Vec3(  0.0f,0.10f,-29.0f),
        Vec3(-15.0f,0.10f,-1.0f)
    };
    const ObjectTransform& t=artXform[selectedArt];
    Vec3 p=base[selectedArt]+Vec3(t.tx,0,t.tz);

    glPushMatrix();
    glTranslatef(p.x,0.16f,p.z);
    glRotatef(90.0f,1,0,0);
    setMaterial(0.92f,0.64f,0.12f,80,0.85f);
    GLfloat e[]={0.28f,0.15f,0.02f,1.0f};
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,e);
    glutSolidTorus(0.055f,2.75f*clampf(t.scale,0.75f,1.25f),18,52);
    resetEmission();
    glPopMatrix();
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
    if(floorY > SECOND_FLOOR_Y-1.2f) return "Level 2 - Grand Exhibition Hall";
    if(floorY > 0.85f) return "Grand Staircase";
    if(cameraPos.z > 36.2f) return "Entrance Plaza / Forecourt";
    if(cameraPos.z > 24.0f) return "Grand Lobby";
    if(cameraPos.z < -24.0f) return "Sculpture Hall";
    if(std::fabs(cameraPos.x) < 6.0f) return "Main Corridor";
    if(cameraPos.x < -6.0f){
        if(cameraPos.z > 8.0f) return "Wildlife Gallery";
        if(cameraPos.z > -10.0f) return "Modern Art Gallery";
        return "Cultural Art Gallery";
    }
    if(cameraPos.z > 8.0f) return "Classical Gallery";
    if(cameraPos.z > -10.0f) return "Nature Gallery";
    return "History & Artifacts Gallery";
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
        const char* names[] = {"Royal Bengal Tiger","Asian Elephant","Spotted Deer","Kinetic Sculpture"};
        drawBitmapText(18,gWinH-48,"W/A/S/D = walk like first person | Mouse = look | V = overview | M = release/capture mouse",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-68,"Walk close to a major exhibit and press E for Smart Museum information.",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-88,"1-4 select exhibit | HOLD J/L = X | I/K = Z | U/O = Y | R/T = rotate | +/- = scale",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-108,"0 reset selected | F1/F2/F3 lights | P animation | H help | ESC exit",GLUT_BITMAP_HELVETICA_12);
        drawBitmapText(18,gWinH-130,std::string("Selected: ")+names[selectedArt]+"  (gold ring shows selected exhibit)",GLUT_BITMAP_HELVETICA_12);
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
    drawTigerArtwork();
    drawElephantArtwork();
    drawDeerArtwork();
    drawAbstractSculpture();
    drawAdditionalGalleryObjects();
    drawSelectedObjectMarker();

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

    return false;
}

void update() {
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=(now-previousTime)/1000.0f;
    previousTime=now;
    dt=std::min(dt,0.05f);

    if(animateScene) {
        centralRotation += 38.0f*dt;
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

    // Initial art transformations can be edited interactively.
    artXform[0].rotY=0;
    artXform[1].rotY=0;
    artXform[2].rotY=0;
    artXform[3].rotY=0;

    previousTime=glutGet(GLUT_ELAPSED_TIME);
}

int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWinW,gWinH);
    glutInitWindowPosition(80,45);
    int museumWindow = glutCreateWindow("Interactive 3D Smart Museum & Art Gallery - CSE 444");
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
