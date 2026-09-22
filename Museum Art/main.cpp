#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

// ================================================================
// Interactive 3D Smart Museum & Art Gallery
// Animal & Exhibit Polish Version / Commit 5
// CSE 444 Computer Graphics Project
//
// Development added after Commit 4:
// - Redesigned Tiger with a longer feline body, white belly/muzzle and visible stripes
// - Improved Asian Elephant with natural gray shades, bigger ears, trunk and tusks
// - Improved Spotted Deer with slimmer body, white spots and clearer antlers
// - Added richer museum-style animal information cards
// - Added small spotlight fixtures above major animal exhibits
// - Existing multi-object transform controls, HUD and light toggles retained
// - Existing galleries, artwork, benches, plants and decorations retained
//
// Later commits can add more room structure, textures, staircase,
// second floor and Smart Museum interactions.
// ================================================================

constexpr float PI = 3.14159265358979323846f;

struct Vec3 {
    float x, y, z;
    Vec3(float X=0, float Y=0, float Z=0) : x(X), y(Y), z(Z) {}
};

// -------------------- Window / Camera -----------------------------
int winW = 1280;
int winH = 720;

Vec3 cameraPos(0.0f, 1.70f, 42.0f);
float yawAngle = -90.0f;
float pitchAngle = 0.0f;
bool mouseLook = true;
bool firstMouse = true;
int lastMouseX = 0;
int lastMouseY = 0;

bool keyDown[256] = {false};

// -------------------- Scene state --------------------------------
struct ExhibitTransform {
    float tx, ty, tz;
    float rotY;
    float scale;
    ExhibitTransform() : tx(0), ty(0), tz(0), rotY(0), scale(1.0f) {}
};

// 0 = Tiger, 1 = Elephant, 2 = Deer, 3 = Kinetic Sculpture
ExhibitTransform artXform[4];
int selectedExhibit = 0;

bool animateScene = true;
float fanAngle = 0.0f;

bool lightEnabled[3] = {true,true,true};
bool overviewMode = false;
Vec3 savedCameraPos;
float savedYaw = -90.0f;
float savedPitch = 0.0f;

// -------------------- Helpers ------------------------------------
float degToRad(float d) { return d * PI / 180.0f; }

void setMaterial(float r, float g, float b,
                 float shininess = 28.0f, float specular = 0.25f) {
    GLfloat ambient[]  = {r*0.30f, g*0.30f, b*0.30f, 1.0f};
    GLfloat diffuse[]  = {r, g, b, 1.0f};
    GLfloat spec[]     = {specular, specular, specular, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void drawBox(float x,float y,float z,
             float sx,float sy,float sz,
             float r,float g,float b,
             float shiny=20.0f,float spec=0.20f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterial(r,g,b,shiny,spec);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawSphere(float x,float y,float z,float radius,
                float r,float g,float b) {
    glPushMatrix();
    glTranslatef(x,y,z);
    setMaterial(r,g,b,24.0f,0.20f);
    glutSolidSphere(radius,24,18);
    glPopMatrix();
}


void drawScaledSphere(float x,float y,float z,
                      float sx,float sy,float sz,
                      float r,float g,float b,
                      float shiny=20.0f,float spec=0.16f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(sx,sy,sz);
    setMaterial(r,g,b,shiny,spec);
    glutSolidSphere(1.0,28,20);
    glPopMatrix();
}

void drawCylinder(float x,float y,float z,
                  float radius,float height,
                  float r,float g,float b) {
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(-90,1,0,0);
    setMaterial(r,g,b,24.0f,0.20f);
    gluCylinder(q,radius,radius,height,20,4);
    gluDisk(q,0,radius,20,1);
    glTranslatef(0,0,height);
    gluDisk(q,0,radius,20,1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawStrokeText(const std::string& text,
                    float x,float y,float z,
                    float scale,
                    float r=0.05f,float g=0.05f,float b=0.05f) {
    glDisable(GL_LIGHTING);
    glColor3f(r,g,b);
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(scale,scale,scale);
    for(char c : text) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawFrame(float x,float y,float z,float w,float h,
               float pr,float pg,float pb) {
    // wooden frame
    drawBox(x,y+h*0.5f,z,w+0.30f,0.15f,0.12f,0.30f,0.12f,0.04f);
    drawBox(x,y-h*0.5f,z,w+0.30f,0.15f,0.12f,0.30f,0.12f,0.04f);
    drawBox(x-w*0.5f,y,z,0.15f,h,0.12f,0.30f,0.12f,0.04f);
    drawBox(x+w*0.5f,y,z,0.15f,h,0.12f,0.30f,0.12f,0.04f);
    // simple procedural artwork panel
    drawBox(x,y,z+0.065f,w,h,0.04f,pr,pg,pb,8.0f,0.05f);
}

void drawWallArtFront(float x,float y,float z,float w,float h,
                      float r,float g,float b) {
    drawFrame(x,y,z,w,h,r,g,b);
    // simple geometric decoration
    drawSphere(x,y,z+0.12f,0.30f,0.92f-r*0.2f,0.70f,0.18f);
}

void drawBench(float x,float z,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotY,0,1,0);
    drawBox(0,0.65f,0,3.3f,0.25f,0.90f,0.36f,0.16f,0.06f);
    drawBox(0,1.35f,0.36f,3.3f,1.15f,0.20f,0.30f,0.12f,0.04f);
    drawBox(-1.25f,0.25f,0,0.22f,0.75f,0.55f,0.22f,0.09f,0.03f);
    drawBox( 1.25f,0.25f,0,0.22f,0.75f,0.55f,0.22f,0.09f,0.03f);
    glPopMatrix();
}

void drawPlant(float x,float z) {
    drawCylinder(x,0,z,0.45f,0.55f,0.42f,0.22f,0.08f);
    drawSphere(x,1.05f,z,0.55f,0.10f,0.45f,0.14f);
    drawSphere(x-0.35f,1.15f,z,0.35f,0.08f,0.38f,0.12f);
    drawSphere(x+0.35f,1.15f,z,0.35f,0.08f,0.38f,0.12f);
    drawSphere(x,1.45f,z,0.38f,0.12f,0.52f,0.18f);
}

// -------------------- Basic exhibit models ------------------------
void drawSimpleTiger() {
    // Commit 5: improved Royal Bengal Tiger built from OpenGL primitives.
    const float OR  = 0.92f, OG  = 0.34f, OB  = 0.055f;
    const float OR2 = 0.76f, OG2 = 0.22f, OB2 = 0.030f;
    const float WHR = 0.96f, WHG = 0.86f, WHB = 0.68f;
    const float BK  = 0.025f;

    glPushMatrix();
    glTranslatef(artXform[0].tx,artXform[0].ty,artXform[0].tz);
    glRotatef(artXform[0].rotY,0,1,0);
    glScalef(artXform[0].scale,artXform[0].scale,artXform[0].scale);

    // Long body + shoulder/haunch masses
    drawScaledSphere(-0.25f,1.28f,0.0f,1.62f,0.68f,0.58f,OR,OG,OB,18,0.10f);
    drawScaledSphere( 0.88f,1.38f,0.0f,0.76f,0.70f,0.58f,OR2,OG2,OB2,18,0.10f);
    drawScaledSphere(-1.28f,1.30f,0.0f,0.72f,0.72f,0.58f,OR2,OG2,OB2,18,0.10f);

    // White/cream underside and chest
    drawScaledSphere(-0.10f,0.96f,0.0f,1.18f,0.24f,0.50f,WHR,WHG,WHB,10,0.05f);
    drawScaledSphere( 1.02f,1.25f,0.0f,0.38f,0.48f,0.46f,WHR,WHG,WHB,10,0.05f);

    // Neck and head
    drawScaledSphere(1.18f,1.68f,0.0f,0.58f,0.62f,0.50f,OR,OG,OB,18,0.10f);
    drawScaledSphere(1.78f,1.88f,0.0f,0.64f,0.56f,0.52f,OR,OG,OB,18,0.10f);

    // Cheeks and muzzle
    drawScaledSphere(2.18f,1.76f, 0.22f,0.38f,0.27f,0.26f,WHR,WHG,WHB,10,0.04f);
    drawScaledSphere(2.18f,1.76f,-0.22f,0.38f,0.27f,0.26f,WHR,WHG,WHB,10,0.04f);
    drawScaledSphere(2.42f,1.73f,0.0f,0.17f,0.12f,0.18f,BK,BK,BK,8,0.02f);

    // Eyes
    drawSphere(1.98f,2.04f, 0.39f,0.075f,0.92f,0.68f,0.15f);
    drawSphere(1.98f,2.04f,-0.39f,0.075f,0.92f,0.68f,0.15f);
    drawSphere(2.03f,2.04f, 0.395f,0.035f,BK,BK,BK);
    drawSphere(2.03f,2.04f,-0.395f,0.035f,BK,BK,BK);

    // Ears
    for(float side : {-1.0f,1.0f}) {
        glPushMatrix();
        glTranslatef(1.55f,2.38f,0.34f*side);
        glRotatef(-90,1,0,0);
        setMaterial(BK,BK,BK,10,0.04f);
        glutSolidCone(0.22f,0.50f,18,7);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(1.57f,2.40f,0.34f*side);
        glRotatef(-90,1,0,0);
        setMaterial(0.88f,0.45f,0.28f,8,0.03f);
        glutSolidCone(0.13f,0.34f,16,6);
        glPopMatrix();
    }

    // Four legs + paws
    const float legX[4] = {-1.05f,-0.48f,0.58f,1.05f};
    const float legZ[4] = { 0.42f,-0.42f,0.42f,-0.42f};
    for(int i=0;i<4;i++) {
        drawScaledSphere(legX[i],0.82f,legZ[i],0.25f,0.42f,0.23f,OR2,OG2,OB2,12,0.05f);
        drawCylinder(legX[i],0.15f,legZ[i],0.14f,0.62f,OR,OG,OB);
        drawScaledSphere(legX[i]+0.08f,0.12f,legZ[i],0.30f,0.13f,0.22f,OR2,OG2,OB2,10,0.04f);
    }

    // Tail built as visible curved segments
    for(int i=0;i<7;i++) {
        float tx = -1.70f - 0.24f*i;
        float ty = 1.35f + 0.14f*i - 0.018f*i*i;
        float tz = 0.06f*i;
        bool dark = (i==3 || i==5 || i==6);
        drawScaledSphere(tx,ty,tz,0.22f,0.13f,0.13f,
                         dark?BK:OR2,
                         dark?BK:OG2,
                         dark?BK:OB2,10,0.03f);
    }

    // Bengal stripes on both visible body sides
    const float stripeX[7] = {-1.20f,-0.85f,-0.45f,-0.05f,0.35f,0.70f,1.02f};
    for(int i=0;i<7;i++) {
        float h = 0.34f + 0.06f*(i%3);
        drawBox(stripeX[i],1.54f, 0.585f,0.10f,h,0.045f,BK,BK,BK,6,0.02f);
        drawBox(stripeX[i],1.54f,-0.585f,0.10f,h,0.045f,BK,BK,BK,6,0.02f);
    }

    // Face stripes
    drawBox(1.62f,2.12f, 0.48f,0.08f,0.26f,0.035f,BK,BK,BK,6,0.02f);
    drawBox(1.62f,2.12f,-0.48f,0.08f,0.26f,0.035f,BK,BK,BK,6,0.02f);
    drawBox(1.92f,2.18f, 0.46f,0.07f,0.20f,0.035f,BK,BK,BK,6,0.02f);
    drawBox(1.92f,2.18f,-0.46f,0.07f,0.20f,0.035f,BK,BK,BK,6,0.02f);

    glPopMatrix();
}


void drawSimpleElephant(float x,float z) {
    // Commit 5: improved Asian Elephant with natural gray shades.
    const float G  = 0.43f;
    const float G2 = 0.35f;
    const float G3 = 0.50f;
    const float IV = 0.92f;

    glPushMatrix();
    glTranslatef(x,0.50f,z);
    glTranslatef(artXform[1].tx,artXform[1].ty,artXform[1].tz);
    glRotatef(artXform[1].rotY,0,1,0);
    glScalef(artXform[1].scale,artXform[1].scale,artXform[1].scale);

    // Barrel body, shoulder and rump
    drawScaledSphere(-0.28f,1.35f,0.0f,1.65f,1.00f,0.83f,G,G,G+0.02f,14,0.06f);
    drawScaledSphere( 0.95f,1.48f,0.0f,0.82f,0.92f,0.76f,G2,G2,G2+0.02f,14,0.06f);
    drawScaledSphere(-1.45f,1.32f,0.0f,0.72f,0.88f,0.72f,G,G,G+0.02f,14,0.06f);

    // Head
    drawScaledSphere(1.72f,1.85f,0.0f,0.78f,0.80f,0.68f,G3,G3,G3+0.02f,14,0.06f);
    drawScaledSphere(1.88f,2.28f,0.0f,0.50f,0.40f,0.52f,G3,G3,G3+0.02f,12,0.05f);

    // Broad Asian elephant ears
    for(float side : {-1.0f,1.0f}) {
        drawScaledSphere(1.25f,1.88f,0.64f*side,
                         0.56f,0.72f,0.10f,
                         0.39f,0.36f,0.37f,10,0.04f);
        drawScaledSphere(1.28f,1.88f,0.69f*side,
                         0.38f,0.50f,0.045f,
                         0.54f,0.43f,0.43f,8,0.03f);
    }

    // Four legs with broad feet
    const float lx[4] = {-1.00f,-0.45f,0.55f,1.00f};
    const float lz[4] = { 0.52f,-0.52f,0.52f,-0.52f};
    for(int i=0;i<4;i++) {
        drawCylinder(lx[i],0.10f,lz[i],0.22f,0.90f,G2,G2,G2);
        drawScaledSphere(lx[i],0.12f,lz[i],0.36f,0.18f,0.33f,0.34f,0.35f,0.37f,10,0.03f);
    }

    // Segmented curved trunk
    for(int i=0;i<6;i++) {
        float ty = 1.52f - 0.24f*i;
        float tx = 2.16f + 0.08f*i;
        float radius = 0.20f - 0.018f*i;
        drawScaledSphere(tx,ty,0.0f,radius,radius*1.15f,radius,G2,G2,G2,10,0.03f);
    }
    drawScaledSphere(2.62f,0.30f,0.0f,0.16f,0.12f,0.15f,G2,G2,G2,10,0.03f);

    // Tusks
    for(float side : {-1.0f,1.0f}) {
        glPushMatrix();
        glTranslatef(2.12f,1.55f,0.34f*side);
        glRotatef(-22,0,0,1);
        glRotatef(side*12,0,1,0);
        setMaterial(IV,IV,0.76f,18,0.06f);
        glutSolidCone(0.09f,0.62f,18,7);
        glPopMatrix();
    }

    // Eyes
    drawSphere(2.03f,2.02f, 0.49f,0.060f,0.025f,0.022f,0.020f);
    drawSphere(2.03f,2.02f,-0.49f,0.060f,0.025f,0.022f,0.020f);

    // Tail
    drawCylinder(-1.95f,1.10f,0.0f,0.055f,0.72f,G2,G2,G2);
    drawScaledSphere(-1.95f,1.02f,0.0f,0.12f,0.24f,0.12f,0.10f,0.08f,0.06f,8,0.02f);

    glPopMatrix();
}

void drawReceptionDesk() {
    drawBox(0,0.55f,6.2f,7.2f,1.10f,1.6f,0.34f,0.16f,0.07f,32,0.30f);
    drawBox(0,1.20f,6.2f,7.5f,0.22f,1.9f,0.48f,0.26f,0.10f,34,0.30f);
    drawStrokeText("RECEPTION",-2.10f,1.34f,5.34f,0.0031f,0.92f,0.82f,0.54f);
}

void drawSimpleClock() {
    glPushMatrix();
    glTranslatef(0,5.75f,11.45f);
    glRotatef(180,0,1,0);

    // face and rim
    drawCylinder(0,0,0,1.05f,0.13f,0.14f,0.18f,0.24f);
    drawCylinder(0,0,-0.02f,0.87f,0.15f,0.92f,0.88f,0.72f);

    // hands
    glDisable(GL_LIGHTING);
    glColor3f(0.08f,0.07f,0.05f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex3f(0,0,0.17f); glVertex3f(0.0f,0.58f,0.17f);
    glVertex3f(0,0,0.17f); glVertex3f(0.42f,0.0f,0.17f);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawWallLamp(float x,float y,float z,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);
    drawBox(0,0,0,0.18f,0.55f,0.22f,0.32f,0.22f,0.10f,30,0.25f);
    drawSphere(0,0.35f,0.08f,0.17f,0.95f,0.78f,0.40f);
    glPopMatrix();
}


void drawSideWallArt(float x,float y,float z,float w,float h,
                     float r,float g,float b,float rotY) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // frame
    drawBox(0,0,0,w+0.34f,h+0.34f,0.16f,0.25f,0.11f,0.04f,28,0.22f);
    // artwork surface
    drawBox(0,0,0.10f,w,h,0.05f,r,g,b,18,0.08f);

    glPopMatrix();
}

void drawPictureLight(float x,float y,float z,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    drawBox(0,0,0,1.30f,0.10f,0.12f,0.30f,0.22f,0.10f,30,0.22f);
    drawCylinder(-0.48f,-0.18f,0.02f,0.035f,0.22f,0.38f,0.28f,0.12f);
    drawCylinder( 0.48f,-0.18f,0.02f,0.035f,0.22f,0.38f,0.28f,0.12f);
    drawSphere(0,-0.23f,0.08f,0.08f,0.98f,0.82f,0.45f);

    glPopMatrix();
}

void drawMuseumPlaque(const std::string& text,
                      float x,float y,float z,float rotY=0.0f,
                      float width=3.5f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    drawBox(0,0,0,width,0.62f,0.09f,0.94f,0.93f,0.88f,12,0.05f);

    glTranslatef(-width*0.43f,-0.10f,0.06f);
    glScalef(0.00155f,0.00155f,0.00155f);
    setMaterial(0.03f,0.03f,0.03f,6,0.02f);
    for(char c : text) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);

    glPopMatrix();
}

void drawRopeBarrier(float x1,float z1,float x2,float z2) {
    // Posts
    for(int i=0;i<2;i++) {
        float x = (i==0)?x1:x2;
        float z = (i==0)?z1:z2;
        drawCylinder(x,0.0f,z,0.07f,0.92f,0.62f,0.44f,0.12f);
        drawSphere(x,0.96f,z,0.12f,0.74f,0.56f,0.17f);
        drawBox(x,0.03f,z,0.42f,0.06f,0.42f,0.34f,0.24f,0.12f);
    }

    // Simple rope made from small spheres along the line.
    const int segments = 16;
    for(int i=0;i<=segments;i++) {
        float t = i/(float)segments;
        float x = x1 + (x2-x1)*t;
        float z = z1 + (z2-z1)*t;
        float sag = 0.80f - 0.20f*std::sin(PI*t);
        drawSphere(x,sag,z,0.055f,0.46f,0.06f,0.05f);
    }
}

void drawSimpleDeer(float x,float z) {
    // Commit 5: improved Spotted Deer / Chital.
    const float BR  = 0.58f, BG  = 0.28f, BB  = 0.08f;
    const float BR2 = 0.43f, BG2 = 0.18f, BB2 = 0.045f;
    const float WHR = 0.95f, WHG = 0.88f, WHB = 0.72f;

    glPushMatrix();
    glTranslatef(x,0.50f,z);
    glTranslatef(artXform[2].tx,artXform[2].ty,artXform[2].tz);
    glRotatef(artXform[2].rotY,0,1,0);
    glScalef(artXform[2].scale,artXform[2].scale,artXform[2].scale);

    // Slender body
    drawScaledSphere(-0.20f,1.38f,0.0f,1.35f,0.64f,0.52f,BR,BG,BB,14,0.06f);
    drawScaledSphere(-1.15f,1.38f,0.0f,0.58f,0.62f,0.50f,BR2,BG2,BB2,14,0.06f);

    // Neck and narrow head
    drawScaledSphere(0.85f,1.73f,0.0f,0.40f,0.78f,0.34f,BR2,BG2,BB2,12,0.05f);
    drawScaledSphere(1.18f,2.28f,0.0f,0.43f,0.48f,0.34f,BR,BG,BB,12,0.05f);
    drawScaledSphere(1.50f,2.18f,0.0f,0.30f,0.25f,0.24f,BR2,BG2,BB2,10,0.04f);

    // White throat/belly
    drawScaledSphere(0.82f,1.62f,0.0f,0.26f,0.54f,0.31f,WHR,WHG,WHB,10,0.04f);
    drawScaledSphere(-0.15f,1.02f,0.0f,0.90f,0.18f,0.43f,WHR,WHG,WHB,10,0.03f);

    // Long slim legs
    const float lx[4] = {-0.92f,-0.48f,0.48f,0.82f};
    const float lz[4] = { 0.34f,-0.34f,0.34f,-0.34f};
    for(int i=0;i<4;i++) {
        drawCylinder(lx[i],0.12f,lz[i],0.075f,0.88f,BR2,BG2,BB2);
        drawScaledSphere(lx[i],0.10f,lz[i],0.15f,0.08f,0.18f,0.055f,0.040f,0.030f,8,0.02f);
    }

    // Ears
    for(float side : {-1.0f,1.0f}) {
        glPushMatrix();
        glTranslatef(1.05f,2.68f,0.23f*side);
        glRotatef(-90,1,0,0);
        setMaterial(BR2,BG2,BB2,8,0.03f);
        glutSolidCone(0.13f,0.34f,14,5);
        glPopMatrix();
    }

    // Eyes and nose
    drawSphere(1.38f,2.37f, 0.27f,0.045f,0.02f,0.018f,0.014f);
    drawSphere(1.38f,2.37f,-0.27f,0.045f,0.02f,0.018f,0.014f);
    drawSphere(1.76f,2.17f,0.0f,0.08f,0.055f,0.04f,0.03f);

    // White spots on both sides
    const float sx[12] = {-1.05f,-0.78f,-0.48f,-0.18f,0.10f,0.38f,
                          -0.90f,-0.60f,-0.30f,0.00f,0.28f,0.56f};
    const float sy[12] = {1.58f,1.69f,1.61f,1.72f,1.61f,1.68f,
                          1.28f,1.34f,1.27f,1.36f,1.29f,1.36f};
    for(int i=0;i<12;i++) {
        drawScaledSphere(sx[i],sy[i], 0.52f,0.060f,0.050f,0.025f,0.97f,0.91f,0.78f,6,0.01f);
        drawScaledSphere(sx[i],sy[i],-0.52f,0.060f,0.050f,0.025f,0.97f,0.91f,0.78f,6,0.01f);
    }

    // Antlers: simple branched silhouette
    for(float side : {-1.0f,1.0f}) {
        float az = 0.15f*side;
        drawCylinder(1.03f,2.60f,az,0.035f,0.66f,0.24f,0.13f,0.055f);
        drawCylinder(1.16f,2.95f,az,0.030f,0.38f,0.24f,0.13f,0.055f);
        drawCylinder(0.92f,2.90f,az,0.028f,0.32f,0.24f,0.13f,0.055f);
    }

    // Tail with light tip
    drawScaledSphere(-1.55f,1.58f,0.0f,0.16f,0.30f,0.12f,BR2,BG2,BB2,8,0.03f);
    drawScaledSphere(-1.60f,1.74f,0.0f,0.10f,0.15f,0.08f,WHR,WHG,WHB,8,0.02f);

    glPopMatrix();
}


void drawAnimalInfoCard(const std::string& line1,
                        const std::string& line2,
                        float x,float y,float z,float rotY=0.0f,
                        float width=4.8f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    // ivory museum card + brass edge
    drawBox(0,0,0,width,0.96f,0.10f,0.96f,0.95f,0.90f,14,0.05f);
    drawBox(0, 0.45f,0.06f,width,0.035f,0.03f,0.58f,0.43f,0.16f,20,0.12f);
    drawBox(0,-0.45f,0.06f,width,0.035f,0.03f,0.58f,0.43f,0.16f,20,0.12f);

    drawStrokeText(line1,-width*0.43f,0.10f,0.07f,0.00155f,0.05f,0.045f,0.04f);
    drawStrokeText(line2,-width*0.43f,-0.25f,0.07f,0.00110f,0.14f,0.12f,0.10f);

    glPopMatrix();
}

void drawExhibitSpotlight(float x,float y,float z,float rotY=0.0f) {
    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(rotY,0,1,0);

    drawBox(0,0,0,0.42f,0.24f,0.32f,0.16f,0.14f,0.12f,32,0.22f);
    glPushMatrix();
    glTranslatef(0,-0.18f,0.12f);
    glRotatef(90,1,0,0);
    setMaterial(0.92f,0.78f,0.42f,45,0.35f);
    glutSolidCone(0.20f,0.34f,18,6);
    glPopMatrix();

    glPopMatrix();
}

void drawGeometricSculpture(float x,float z,float rot=0.0f) {
    drawBox(x,0.35f,z,2.4f,0.70f,2.4f,0.91f,0.90f,0.85f);

    glPushMatrix();
    glTranslatef(x,1.65f,z);
    glRotatef(rot,0,1,0);

    setMaterial(0.72f,0.22f,0.12f,60,0.55f);
    glutSolidTorus(0.13f,0.72f,18,32);

    glRotatef(90,1,0,0);
    setMaterial(0.12f,0.38f,0.68f,60,0.55f);
    glutSolidTorus(0.11f,0.52f,16,28);

    drawSphere(0,0,0,0.24f,0.78f,0.58f,0.18f);
    glPopMatrix();
}

void drawSimpleBust(float x,float z) {
    drawBox(x,0.45f,z,1.20f,0.90f,1.20f,0.90f,0.88f,0.80f);
    drawCylinder(x,0.90f,z,0.30f,0.45f,0.86f,0.86f,0.84f);
    drawSphere(x,1.65f,z,0.48f,0.86f,0.86f,0.84f);
    drawSphere(x,1.35f,z,0.68f,0.82f,0.82f,0.80f);
}

void drawArtifact(float x,float z) {
    drawBox(x,0.45f,z,1.50f,0.90f,1.50f,0.92f,0.91f,0.86f);
    drawCylinder(x,0.90f,z,0.34f,1.00f,0.52f,0.26f,0.08f);
    drawSphere(x,1.95f,z,0.42f,0.62f,0.32f,0.10f);
}

// -------------------- Building -----------------------------------
void drawExterior() {
    // plaza
    drawBox(0,-0.08f,18.0f,34.0f,0.15f,32.0f,0.58f,0.58f,0.60f,8,0.05f);

    // front facade
    drawBox(-10.0f,4.0f,12.0f,12.0f,8.0f,0.55f,0.80f,0.78f,0.70f);
    drawBox( 10.0f,4.0f,12.0f,12.0f,8.0f,0.55f,0.80f,0.78f,0.70f);
    drawBox(0,7.35f,12.0f,8.0f,1.30f,0.55f,0.80f,0.78f,0.70f);

    // entrance frame
    drawBox(-4.25f,3.50f,11.70f,0.45f,7.0f,0.60f,0.28f,0.12f,0.05f);
    drawBox( 4.25f,3.50f,11.70f,0.45f,7.0f,0.60f,0.28f,0.12f,0.05f);
    drawBox(0,6.80f,11.70f,8.5f,0.45f,0.60f,0.28f,0.12f,0.05f);

    drawStrokeText("SMART MUSEUM",-4.0f,8.10f,11.35f,0.0065f,0.18f,0.12f,0.05f);

    // simple columns
    for(float x : {-14.5f,14.5f}) {
        drawCylinder(x,0,11.0f,0.65f,7.0f,0.90f,0.88f,0.82f);
        drawBox(x,0.20f,11.0f,1.80f,0.40f,1.80f,0.86f,0.84f,0.78f);
        drawBox(x,7.0f,11.0f,1.80f,0.40f,1.80f,0.86f,0.84f,0.78f);
    }
}

void drawMuseumShell() {
    // floor / ceiling
    drawBox(0,-0.12f,-12.0f,48.0f,0.20f,50.0f,0.88f,0.87f,0.84f,10,0.05f);
    drawBox(0,8.05f,-12.0f,48.0f,0.20f,50.0f,0.32f,0.32f,0.34f,10,0.05f);

    // side/back walls
    drawBox(-24.0f,4.0f,-12.0f,0.30f,8.0f,50.0f,0.78f,0.76f,0.71f);
    drawBox( 24.0f,4.0f,-12.0f,0.30f,8.0f,50.0f,0.78f,0.76f,0.71f);
    drawBox(0,4.0f,-37.0f,48.0f,8.0f,0.30f,0.78f,0.76f,0.71f);

    // simple internal partitions leaving openings
    drawBox(-11.0f,4.0f,-2.0f,0.30f,8.0f,22.0f,0.82f,0.80f,0.76f);
    drawBox( 11.0f,4.0f,-2.0f,0.30f,8.0f,22.0f,0.82f,0.80f,0.76f);
    drawBox(-11.0f,4.0f,-28.0f,0.30f,8.0f,18.0f,0.82f,0.80f,0.76f);
    drawBox( 11.0f,4.0f,-28.0f,0.30f,8.0f,18.0f,0.82f,0.80f,0.76f);

    // front interior wall segments around entrance
    drawBox(-14.0f,4.0f,12.0f,20.0f,8.0f,0.30f,0.80f,0.78f,0.72f);
    drawBox( 14.0f,4.0f,12.0f,20.0f,8.0f,0.30f,0.80f,0.78f,0.72f);
}

void drawInteriorDecoration() {
    // central corridor carpet
    drawBox(0,0.02f,-11.0f,7.0f,0.04f,44.0f,0.42f,0.04f,0.07f,6,0.02f);

    // Commit 3: thin decorative floor borders along corridor
    drawBox(-3.72f,0.035f,-11.0f,0.16f,0.05f,44.0f,0.74f,0.55f,0.18f,18,0.12f);
    drawBox( 3.72f,0.035f,-11.0f,0.16f,0.05f,44.0f,0.74f,0.55f,0.18f,18,0.12f);

    // entrance / reception
    drawReceptionDesk();
    drawSimpleClock();

    // room headings
    drawStrokeText("WILDLIFE GALLERY",-22.6f,7.0f,2.0f,0.0036f,0.18f,0.11f,0.05f);
    drawStrokeText("CLASSICAL GALLERY",12.5f,7.0f,2.0f,0.0034f,0.18f,0.11f,0.05f);
    drawStrokeText("HISTORY GALLERY",-22.2f,7.0f,-24.5f,0.0035f,0.18f,0.11f,0.05f);
    drawStrokeText("NATURE GALLERY",13.2f,7.0f,-24.5f,0.0035f,0.18f,0.11f,0.05f);

    // front/back wall art
    drawWallArtFront(-17.0f,5.0f,10.82f,4.2f,3.0f,0.15f,0.48f,0.30f);
    drawWallArtFront(-7.2f,4.9f,10.82f,3.8f,2.7f,0.52f,0.22f,0.12f);
    drawWallArtFront( 7.2f,4.9f,10.82f,3.8f,2.7f,0.20f,0.40f,0.70f);
    drawWallArtFront(17.0f,5.0f,10.82f,4.2f,3.0f,0.68f,0.18f,0.16f);

    drawWallArtFront(-17.0f,5.0f,-36.82f,4.2f,3.0f,0.12f,0.38f,0.65f);
    drawWallArtFront(-6.0f,5.0f,-36.82f,4.5f,3.1f,0.20f,0.62f,0.58f);
    drawWallArtFront( 6.0f,5.0f,-36.82f,4.5f,3.1f,0.62f,0.24f,0.56f);
    drawWallArtFront(17.0f,5.0f,-36.82f,4.2f,3.0f,0.55f,0.44f,0.14f);

    // Commit 3: side-wall paintings
    drawSideWallArt(-23.80f,5.0f, 3.8f,4.2f,2.8f,0.52f,0.18f,0.12f, 90.0f);
    drawSideWallArt(-23.80f,5.0f,-11.5f,4.0f,2.8f,0.18f,0.44f,0.20f, 90.0f);
    drawSideWallArt(-23.80f,5.0f,-27.0f,4.2f,2.8f,0.32f,0.24f,0.62f, 90.0f);

    drawSideWallArt( 23.80f,5.0f, 3.8f,4.2f,2.8f,0.18f,0.34f,0.70f,-90.0f);
    drawSideWallArt( 23.80f,5.0f,-11.5f,4.0f,2.8f,0.66f,0.28f,0.16f,-90.0f);
    drawSideWallArt( 23.80f,5.0f,-27.0f,4.2f,2.8f,0.15f,0.54f,0.50f,-90.0f);

    // artwork title plaques
    drawMuseumPlaque("FOREST STUDY",-23.58f,3.15f, 3.8f, 90.0f,3.1f);
    drawMuseumPlaque("WILDLIFE COLORS",-23.58f,3.15f,-11.5f,90.0f,3.3f);
    drawMuseumPlaque("HERITAGE FORM",-23.58f,3.15f,-27.0f,90.0f,3.2f);

    drawMuseumPlaque("BLUE HORIZON", 23.58f,3.15f, 3.8f,-90.0f,3.0f);
    drawMuseumPlaque("MODERN RHYTHM",23.58f,3.15f,-11.5f,-90.0f,3.2f);
    drawMuseumPlaque("NATURE MEMORY",23.58f,3.15f,-27.0f,-90.0f,3.2f);

    // picture lights above side paintings
    for(float z : {3.8f,-11.5f,-27.0f}) {
        drawPictureLight(-23.55f,6.75f,z,90.0f);
        drawPictureLight( 23.55f,6.75f,z,-90.0f);
    }

    // wall lamps
    for(float z : {5.5f,-8.0f,-21.5f}) {
        drawWallLamp(-23.72f,4.0f,z,90.0f);
        drawWallLamp( 23.72f,4.0f,z,-90.0f);
    }

    // seating stays near walls
    drawBench(-19.8f,-5.5f,90.0f);
    drawBench( 19.8f,-19.0f,-90.0f);

    // plants
    drawPlant(-21.5f,7.5f);
    drawPlant( 21.5f,7.5f);
    drawPlant(-21.5f,-31.0f);
    drawPlant( 21.5f,-31.0f);

    // Wildlife exhibit: Tiger
    drawBox(-16.0f,0.35f,-8.0f,8.2f,0.50f,5.2f,0.90f,0.89f,0.84f);
    glPushMatrix();
    glTranslatef(-16.0f,0.55f,-8.0f);
    drawSimpleTiger();
    glPopMatrix();
    drawStrokeText("ROYAL BENGAL TIGER",-18.2f,0.48f,-5.35f,0.0022f,0.05f,0.05f,0.05f);
    drawRopeBarrier(-19.0f,-4.9f,-13.0f,-4.9f);
    drawAnimalInfoCard("ROYAL BENGAL TIGER","WILDLIFE COLLECTION",-15.9f,1.05f,-4.45f,0.0f,4.8f);
    drawExhibitSpotlight(-16.0f,7.55f,-8.0f);

    // Nature exhibit: Elephant
    drawBox(16.0f,0.35f,-26.0f,8.0f,0.50f,5.0f,0.90f,0.89f,0.84f);
    drawSimpleElephant(16.0f,-26.0f);
    drawStrokeText("ASIAN ELEPHANT",13.9f,0.48f,-23.35f,0.0022f,0.05f,0.05f,0.05f);
    drawRopeBarrier(13.0f,-22.9f,19.0f,-22.9f);
    drawAnimalInfoCard("ASIAN ELEPHANT","NATURE COLLECTION",16.0f,1.05f,-22.45f,0.0f,4.5f);
    drawExhibitSpotlight(16.0f,7.55f,-26.0f);

    // Commit 3: Spotted Deer exhibit in rear center
    drawBox(0.0f,0.35f,-31.0f,7.2f,0.50f,4.4f,0.90f,0.89f,0.84f);
    drawSimpleDeer(0.0f,-31.0f);
    drawStrokeText("SPOTTED DEER",-1.65f,0.48f,-28.65f,0.00225f,0.05f,0.05f,0.05f);
    drawRopeBarrier(-2.6f,-28.45f,2.6f,-28.45f);
    drawAnimalInfoCard("SPOTTED DEER","WILDLIFE COLLECTION",0.0f,1.05f,-28.05f,0.0f,4.2f);
    drawExhibitSpotlight(0.0f,7.55f,-31.0f);

    // Classical area
    drawSimpleBust(15.0f,-8.0f);
    drawSimpleBust(18.0f,-8.0f);
    drawSimpleBust(16.5f,-12.0f);
    drawMuseumPlaque("CLASSICAL STUDIES",16.5f,0.85f,-14.0f,0.0f,3.8f);

    // History artifacts
    drawArtifact(-17.5f,-26.0f);
    drawArtifact(-14.2f,-26.0f);
    drawArtifact(-15.8f,-30.0f);
    drawMuseumPlaque("HISTORICAL OBJECTS",-15.8f,0.85f,-32.2f,0.0f,4.1f);

    // Additional small geometric sculptures
    drawGeometricSculpture(-16.0f,-17.0f,fanAngle*0.35f);
    drawGeometricSculpture( 16.0f,-17.0f,-fanAngle*0.30f);

    // central animated sculpture
    glPushMatrix();
    glTranslatef(0,1.45f,-20.0f);
    glTranslatef(artXform[3].tx,artXform[3].ty,artXform[3].tz);
    glRotatef(artXform[3].rotY,0,1,0);
    glScalef(artXform[3].scale,artXform[3].scale,artXform[3].scale);
    glRotatef(fanAngle,0,1,0);
    setMaterial(0.72f,0.48f,0.12f,64,0.55f);
    glutSolidTorus(0.18f,1.15f,20,40);
    glRotatef(90,1,0,0);
    setMaterial(0.16f,0.32f,0.68f,64,0.55f);
    glutSolidTorus(0.16f,0.82f,18,36);
    glPopMatrix();
    drawBox(0,0.35f,-20.0f,3.5f,0.7f,3.5f,0.92f,0.90f,0.84f);
    drawStrokeText("KINETIC SCULPTURE",-1.95f,0.68f,-18.15f,0.0021f,0.05f,0.05f,0.05f);

    // small direction boards in the central corridor
    drawMuseumPlaque("< WILDLIFE", -4.2f,2.65f,0.0f, 0.0f,2.8f);
    drawMuseumPlaque("CLASSICAL >",  4.2f,2.65f,0.0f, 0.0f,2.8f);
}

void drawCeilingFan(float x,float z) {
    glPushMatrix();
    glTranslatef(x,7.35f,z);
    drawCylinder(0,0,0,0.08f,0.40f,0.35f,0.22f,0.08f);
    glTranslatef(0,-0.15f,0);
    glRotatef(fanAngle,0,1,0);
    for(int i=0;i<4;i++) {
        glPushMatrix();
        glRotatef(i*90.0f,0,1,0);
        drawBox(1.25f,0,0,2.3f,0.08f,0.30f,0.26f,0.13f,0.05f);
        glPopMatrix();
    }
    glPopMatrix();
}


const char* selectedExhibitName() {
    switch(selectedExhibit) {
        case 0: return "ROYAL BENGAL TIGER";
        case 1: return "ASIAN ELEPHANT";
        case 2: return "SPOTTED DEER";
        default: return "KINETIC SCULPTURE";
    }
}

void drawSelectionRing() {
    float x=0.0f, y=0.18f, z=0.0f, radius=2.2f;

    if(selectedExhibit==0) {
        x=-16.0f + artXform[0].tx;
        y=0.62f + artXform[0].ty;
        z=-8.0f + artXform[0].tz;
        radius=2.45f*artXform[0].scale;
    } else if(selectedExhibit==1) {
        x=16.0f + artXform[1].tx;
        y=0.62f + artXform[1].ty;
        z=-26.0f + artXform[1].tz;
        radius=2.45f*artXform[1].scale;
    } else if(selectedExhibit==2) {
        x=0.0f + artXform[2].tx;
        y=0.62f + artXform[2].ty;
        z=-31.0f + artXform[2].tz;
        radius=2.15f*artXform[2].scale;
    } else {
        x=artXform[3].tx;
        y=0.75f + artXform[3].ty;
        z=-20.0f + artXform[3].tz;
        radius=1.65f*artXform[3].scale;
    }

    glPushMatrix();
    glTranslatef(x,y,z);
    glRotatef(90,1,0,0);
    setMaterial(0.95f,0.70f,0.12f,70,0.70f);
    glutSolidTorus(0.055f,radius,14,48);
    glPopMatrix();
}

// -------------------- Lighting -----------------------------------
void setupLights() {
    glEnable(GL_LIGHTING);
    if(lightEnabled[0]) glEnable(GL_LIGHT0); else glDisable(GL_LIGHT0);
    if(lightEnabled[1]) glEnable(GL_LIGHT1); else glDisable(GL_LIGHT1);
    if(lightEnabled[2]) glEnable(GL_LIGHT2); else glDisable(GL_LIGHT2);

    GLfloat globalAmbient[] = {0.16f,0.16f,0.18f,1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,globalAmbient);

    GLfloat amb0[] = {0.16f,0.15f,0.13f,1};
    GLfloat dif0[] = {0.82f,0.78f,0.68f,1};
    GLfloat spe0[] = {0.65f,0.62f,0.55f,1};
    GLfloat pos0[] = {-8.0f,7.0f,6.0f,1};
    glLightfv(GL_LIGHT0,GL_AMBIENT,amb0);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,dif0);
    glLightfv(GL_LIGHT0,GL_SPECULAR,spe0);
    glLightfv(GL_LIGHT0,GL_POSITION,pos0);

    GLfloat amb1[] = {0.08f,0.09f,0.12f,1};
    GLfloat dif1[] = {0.45f,0.55f,0.82f,1};
    GLfloat spe1[] = {0.45f,0.55f,0.82f,1};
    GLfloat pos1[] = {12.0f,6.5f,-20.0f,1};
    glLightfv(GL_LIGHT1,GL_AMBIENT,amb1);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,dif1);
    glLightfv(GL_LIGHT1,GL_SPECULAR,spe1);
    glLightfv(GL_LIGHT1,GL_POSITION,pos1);

    // Commit 2: a soft warm light near the entrance/reception.
    GLfloat amb2[] = {0.06f,0.05f,0.03f,1};
    GLfloat dif2[] = {0.70f,0.48f,0.24f,1};
    GLfloat spe2[] = {0.35f,0.28f,0.18f,1};
    GLfloat pos2[] = {0.0f,6.6f,7.5f,1};
    glLightfv(GL_LIGHT2,GL_AMBIENT,amb2);
    glLightfv(GL_LIGHT2,GL_DIFFUSE,dif2);
    glLightfv(GL_LIGHT2,GL_SPECULAR,spe2);
    glLightfv(GL_LIGHT2,GL_POSITION,pos2);
}

// -------------------- Camera / collision --------------------------
bool insideWall(float x,float z) {
    // outside museum front is allowed
    if(z > 12.0f) return false;

    // outer bounds
    if(x < -23.3f || x > 23.3f || z < -36.3f) return true;

    // simple internal partitions, with doorway gaps
    if(std::fabs(x+11.0f) < 0.42f && z < 8.0f && z > -13.0f) {
        if(!(z > -3.0f && z < 2.5f)) return true;
    }
    if(std::fabs(x-11.0f) < 0.42f && z < 8.0f && z > -13.0f) {
        if(!(z > -3.0f && z < 2.5f)) return true;
    }
    if(std::fabs(x+11.0f) < 0.42f && z < -19.0f && z > -36.0f) {
        if(!(z > -29.0f && z < -24.0f)) return true;
    }
    if(std::fabs(x-11.0f) < 0.42f && z < -19.0f && z > -36.0f) {
        if(!(z > -29.0f && z < -24.0f)) return true;
    }

    return false;
}

void updateCamera(float dt) {
    if(overviewMode) return;
    float speed = 5.0f * dt;
    float yawRad = degToRad(yawAngle);
    float fx = std::cos(yawRad);
    float fz = std::sin(yawRad);
    float rx = -fz;
    float rz = fx;

    float nx = cameraPos.x;
    float nz = cameraPos.z;

    if(keyDown['w'] || keyDown['W']) { nx += fx*speed; nz += fz*speed; }
    if(keyDown['s'] || keyDown['S']) { nx -= fx*speed; nz -= fz*speed; }
    if(keyDown['a'] || keyDown['A']) { nx -= rx*speed; nz -= rz*speed; }
    if(keyDown['d'] || keyDown['D']) { nx += rx*speed; nz += rz*speed; }

    if(!insideWall(nx,cameraPos.z)) cameraPos.x = nx;
    if(!insideWall(cameraPos.x,nz)) cameraPos.z = nz;
}

// -------------------- HUD -----------------------------------------
void drawBitmapText(float x,float y,const std::string& s) {
    glRasterPos2f(x,y);
    for(char c : s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,c);
}

void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0,winW,0,winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // title
    glColor3f(1,1,1);
    drawBitmapText(12,winH-22,"Interactive 3D Smart Museum - Commit 5");
    drawBitmapText(12,winH-42,"WASD: walk | Mouse: look | 1-4: select exhibit | J/L I/K U/O: translate | R/T: rotate | +/-: scale");
    drawBitmapText(12,winH-60,"F1/F2/F3: lights | V: overview | M: mouse look | P: animation | 0: reset selected | ESC: exit");

    // selected exhibit information
    ExhibitTransform &t = artXform[selectedExhibit];
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2)
       << "Selected: " << selectedExhibitName()
       << "   X=" << t.tx
       << " Y=" << t.ty
       << " Z=" << t.tz
       << " Rot=" << t.rotY
       << " Scale=" << t.scale;

    glColor3f(1.0f,0.82f,0.28f);
    drawBitmapText(12,winH-82,ss.str());

    std::string lights = std::string("Lights: [F1 ")
                       + (lightEnabled[0]?"ON":"OFF")
                       + "] [F2 " + (lightEnabled[1]?"ON":"OFF")
                       + "] [F3 " + (lightEnabled[2]?"ON":"OFF") + "]";
    glColor3f(0.82f,0.88f,1.0f);
    drawBitmapText(12,winH-102,lights);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// -------------------- Rendering -----------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,(double)winW/(double)winH,0.1,180.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawRad = degToRad(yawAngle);
    float pitchRad = degToRad(pitchAngle);
    float dx = std::cos(pitchRad)*std::cos(yawRad);
    float dy = std::sin(pitchRad);
    float dz = std::cos(pitchRad)*std::sin(yawRad);

    gluLookAt(cameraPos.x,cameraPos.y,cameraPos.z,
              cameraPos.x+dx,cameraPos.y+dy,cameraPos.z+dz,
              0,1,0);

    setupLights();
    drawExterior();
    drawMuseumShell();
    drawInteriorDecoration();
    drawSelectionRing();
    drawCeilingFan(0,4.0f);
    drawCeilingFan(0,-10.0f);
    drawCeilingFan(0,-28.0f);

    drawHUD();
    glutSwapBuffers();
}

void reshape(int w,int h) {
    winW = (w<1)?1:w;
    winH = (h<1)?1:h;
    glViewport(0,0,winW,winH);
}

// -------------------- Input ---------------------------------------
void keyboardDown(unsigned char key,int,int) {
    keyDown[key] = true;
    ExhibitTransform &t = artXform[selectedExhibit];

    switch(key) {
        case 27:
            std::exit(0);
            break;

        case '1': selectedExhibit=0; break;
        case '2': selectedExhibit=1; break;
        case '3': selectedExhibit=2; break;
        case '4': selectedExhibit=3; break;

        case 'j': case 'J': t.tx -= 0.20f; break;
        case 'l': case 'L': t.tx += 0.20f; break;
        case 'i': case 'I': t.tz -= 0.20f; break;
        case 'k': case 'K': t.tz += 0.20f; break;
        case 'u': case 'U': t.ty += 0.14f; break;
        case 'o': case 'O': t.ty -= 0.14f; break;
        case 'r': case 'R': t.rotY += 6.0f; break;
        case 't': case 'T': t.rotY -= 6.0f; break;
        case '+': case '=': t.scale += 0.06f; break;
        case '-': case '_':
            if(t.scale>0.30f) t.scale -= 0.06f;
            break;

        case '0':
            t = ExhibitTransform();
            break;

        case 'p': case 'P':
            animateScene = !animateScene;
            break;

        case 'm': case 'M':
            mouseLook = !mouseLook;
            firstMouse = true;
            break;

        case 'v': case 'V':
            if(!overviewMode) {
                savedCameraPos = cameraPos;
                savedYaw = yawAngle;
                savedPitch = pitchAngle;
                cameraPos = Vec3(0.0f,18.0f,32.0f);
                yawAngle = -90.0f;
                pitchAngle = -24.0f;
                overviewMode = true;
            } else {
                cameraPos = savedCameraPos;
                yawAngle = savedYaw;
                pitchAngle = savedPitch;
                overviewMode = false;
            }
            firstMouse = true;
            break;
    }
}

void keyboardUp(unsigned char key,int,int) {
    keyDown[key] = false;
}


void specialKeyDown(int key,int,int) {
    if(key==GLUT_KEY_F1) lightEnabled[0] = !lightEnabled[0];
    if(key==GLUT_KEY_F2) lightEnabled[1] = !lightEnabled[1];
    if(key==GLUT_KEY_F3) lightEnabled[2] = !lightEnabled[2];
}

void mouseMotion(int x,int y) {
    if(!mouseLook) return;
    if(firstMouse) {
        lastMouseX=x; lastMouseY=y; firstMouse=false;
        return;
    }

    float dx = float(x-lastMouseX);
    float dy = float(lastMouseY-y);
    lastMouseX=x; lastMouseY=y;

    const float sensitivity=0.15f;
    yawAngle += dx*sensitivity;
    pitchAngle += dy*sensitivity;
    if(pitchAngle>80) pitchAngle=80;
    if(pitchAngle<-80) pitchAngle=-80;
}

void timer(int) {
    static int previous = glutGet(GLUT_ELAPSED_TIME);
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now-previous)/1000.0f;
    previous = now;
    if(dt>0.05f) dt=0.05f;

    updateCamera(dt);
    if(animateScene) fanAngle += 55.0f*dt;
    if(fanAngle>360) fanAngle -= 360;

    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}

void initOpenGL() {
    glClearColor(0.08f,0.09f,0.12f,1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    // Keep fixed-function material behavior simple and stable.
    glDisable(GL_COLOR_MATERIAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW,winH);
    glutInitWindowPosition(80,40);
    glutCreateWindow("Interactive 3D Smart Museum - Commit 5");

    initOpenGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16,timer,0);

    glutMainLoop();
    return 0;
}
