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

// ================================================================
// Interactive 3D Smart Museum & Art Gallery
// Early Improved Version / Commit 2
// CSE 444 Computer Graphics Project
//
// Development added after Commit 1:
// - One-floor museum retained
// - More wall artwork and gallery decoration
// - Reception desk + entrance clock
// - Simple room signs and wall lamps
// - Added a second animal exhibit (Asian Elephant placeholder)
// - More benches/plants and exhibit labels
// - First-person WASD + mouse look
// - Tiger transformation controls retained
// - Basic lighting/materials preserved
//
// Later commits can add improved animals, more rooms, staircase,
// second floor, textures, smart exhibit info, advanced labels, etc.
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
float exhibitX = 0.0f;
float exhibitY = 0.0f;
float exhibitZ = 0.0f;
float exhibitRotY = 0.0f;
float exhibitScale = 1.0f;

bool animateScene = true;
float fanAngle = 0.0f;

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
    // Early placeholder tiger: later commits will redesign it.
    const float OR=0.88f, OG=0.34f, OB=0.05f;
    const float BK=0.03f;
    const float WH=0.88f;

    glPushMatrix();
    glTranslatef(exhibitX,exhibitY,exhibitZ);
    glRotatef(exhibitRotY,0,1,0);
    glScalef(exhibitScale,exhibitScale,exhibitScale);

    drawSphere(0,1.25f,0,1.00f,OR,OG,OB);
    drawSphere(1.10f,1.45f,0,0.62f,OR,OG,OB);
    drawSphere(1.58f,1.40f,0,0.42f,OR,OG,OB);
    drawSphere(1.86f,1.30f,0,0.28f,WH,WH*0.95f,WH*0.78f);

    // ears
    glPushMatrix();
    glTranslatef(1.25f,1.95f,0.35f);
    setMaterial(BK,BK,BK,8,0.05f);
    glRotatef(-90,1,0,0);
    glutSolidCone(0.18f,0.42f,16,6);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(1.25f,1.95f,-0.35f);
    setMaterial(BK,BK,BK,8,0.05f);
    glRotatef(-90,1,0,0);
    glutSolidCone(0.18f,0.42f,16,6);
    glPopMatrix();

    // legs
    for(float lx : {-0.65f,0.55f}) {
        for(float lz : {-0.48f,0.48f}) {
            drawCylinder(lx,0.15f,lz,0.15f,0.85f,OR,OG,OB);
            drawSphere(lx,0.12f,lz,0.22f,OR*0.80f,OG*0.75f,OB);
        }
    }

    // simple dark stripes
    for(float sx : {-0.65f,-0.30f,0.10f,0.45f})
        drawBox(sx,1.42f,0.91f,0.10f,0.55f,0.05f,BK,BK,BK,8,0.02f);

    // tail
    drawCylinder(-1.05f,1.20f,0,0.10f,1.15f,OR,OG,OB);

    glPopMatrix();
}


void drawSimpleElephant(float x,float z) {
    // Commit 2 placeholder elephant: simple but recognizable.
    const float G  = 0.48f;
    const float G2 = 0.40f;
    const float IV = 0.88f;

    glPushMatrix();
    glTranslatef(x,0.50f,z);

    // body + head
    glPushMatrix();
    glScalef(1.70f,1.05f,0.95f);
    drawSphere(0.0f,1.25f,0.0f,0.95f,G,G,G);
    glPopMatrix();

    drawSphere(1.45f,1.45f,0.0f,0.72f,G2,G2,G2);

    // ears
    glPushMatrix();
    glTranslatef(1.25f,1.52f,0.58f);
    glScalef(0.16f,0.72f,0.62f);
    drawSphere(0,0,0,1.0f,0.43f,0.38f,0.38f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.25f,1.52f,-0.58f);
    glScalef(0.16f,0.72f,0.62f);
    drawSphere(0,0,0,1.0f,0.43f,0.38f,0.38f);
    glPopMatrix();

    // legs
    for(float lx : {-0.85f,0.65f}) {
        for(float lz : {-0.52f,0.52f}) {
            drawCylinder(lx,0.05f,lz,0.20f,0.90f,G2,G2,G2);
            drawSphere(lx,0.08f,lz,0.25f,0.36f,0.36f,0.36f);
        }
    }

    // trunk
    drawCylinder(1.83f,0.78f,0.0f,0.13f,1.05f,G2,G2,G2);
    drawSphere(1.83f,0.73f,0.0f,0.16f,G2,G2,G2);

    // tusks
    glPushMatrix();
    glTranslatef(1.72f,1.12f,0.32f);
    glRotatef(15,0,0,1);
    setMaterial(IV,IV,0.72f,18,0.08f);
    glutSolidCone(0.08f,0.65f,16,6);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.72f,1.12f,-0.32f);
    glRotatef(15,0,0,1);
    setMaterial(IV,IV,0.72f,18,0.08f);
    glutSolidCone(0.08f,0.65f,16,6);
    glPopMatrix();

    // eye
    drawSphere(1.80f,1.65f,0.48f,0.055f,0.03f,0.03f,0.03f);

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

    // reception area added in Commit 2
    drawReceptionDesk();
    drawSimpleClock();

    // simple room headings
    drawStrokeText("WILDLIFE GALLERY",-22.6f,7.0f,2.0f,0.0036f,0.18f,0.11f,0.05f);
    drawStrokeText("CLASSICAL GALLERY",12.5f,7.0f,2.0f,0.0034f,0.18f,0.11f,0.05f);
    drawStrokeText("HISTORY GALLERY",-22.2f,7.0f,-24.5f,0.0035f,0.18f,0.11f,0.05f);
    drawStrokeText("NATURE GALLERY",13.2f,7.0f,-24.5f,0.0035f,0.18f,0.11f,0.05f);

    // more wall art than Commit 1
    drawWallArtFront(-17.0f,5.0f,10.82f,4.2f,3.0f,0.15f,0.48f,0.30f);
    drawWallArtFront(-7.2f,4.9f,10.82f,3.8f,2.7f,0.52f,0.22f,0.12f);
    drawWallArtFront( 7.2f,4.9f,10.82f,3.8f,2.7f,0.20f,0.40f,0.70f);
    drawWallArtFront(17.0f,5.0f,10.82f,4.2f,3.0f,0.68f,0.18f,0.16f);

    drawWallArtFront(-17.0f,5.0f,-36.82f,4.2f,3.0f,0.12f,0.38f,0.65f);
    drawWallArtFront(-6.0f,5.0f,-36.82f,4.5f,3.1f,0.20f,0.62f,0.58f);
    drawWallArtFront( 6.0f,5.0f,-36.82f,4.5f,3.1f,0.62f,0.24f,0.56f);
    drawWallArtFront(17.0f,5.0f,-36.82f,4.2f,3.0f,0.55f,0.44f,0.14f);

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

    // Nature exhibit: simple Elephant placeholder added in Commit 2
    drawBox(16.0f,0.35f,-26.0f,8.0f,0.50f,5.0f,0.90f,0.89f,0.84f);
    drawSimpleElephant(16.0f,-26.0f);
    drawStrokeText("ASIAN ELEPHANT",13.9f,0.48f,-23.35f,0.0022f,0.05f,0.05f,0.05f);

    // Classical area
    drawSimpleBust(15.0f,-8.0f);
    drawSimpleBust(18.0f,-8.0f);
    drawSimpleBust(16.5f,-12.0f);

    // History artifacts
    drawArtifact(-17.5f,-26.0f);
    drawArtifact(-14.2f,-26.0f);
    drawArtifact(-15.8f,-30.0f);

    // central animated sculpture
    glPushMatrix();
    glTranslatef(0,1.45f,-20.0f);
    glRotatef(fanAngle,0,1,0);
    setMaterial(0.72f,0.48f,0.12f,64,0.55f);
    glutSolidTorus(0.18f,1.15f,20,40);
    glRotatef(90,1,0,0);
    setMaterial(0.16f,0.32f,0.68f,64,0.55f);
    glutSolidTorus(0.16f,0.82f,18,36);
    glPopMatrix();
    drawBox(0,0.35f,-20.0f,3.5f,0.7f,3.5f,0.92f,0.90f,0.84f);
    drawStrokeText("KINETIC SCULPTURE",-1.95f,0.68f,-18.15f,0.0021f,0.05f,0.05f,0.05f);
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

// -------------------- Lighting -----------------------------------
void setupLights() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

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
    glColor3f(1,1,1);
    drawBitmapText(12,winH-22,"Interactive 3D Smart Museum - Commit 2");
    drawBitmapText(12,winH-42,"WASD: walk | Mouse: look | J/L/I/K/U/O: move Tiger | R/T: rotate | +/-: scale | P: animation | ESC: exit");
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

    switch(key) {
        case 27: std::exit(0); break;
        case 'j': case 'J': exhibitX -= 0.18f; break;
        case 'l': case 'L': exhibitX += 0.18f; break;
        case 'i': case 'I': exhibitZ -= 0.18f; break;
        case 'k': case 'K': exhibitZ += 0.18f; break;
        case 'u': case 'U': exhibitY += 0.12f; break;
        case 'o': case 'O': exhibitY -= 0.12f; break;
        case 'r': case 'R': exhibitRotY += 5.0f; break;
        case 't': case 'T': exhibitRotY -= 5.0f; break;
        case '+': case '=': exhibitScale += 0.05f; break;
        case '-': case '_': exhibitScale = (exhibitScale>0.25f)?exhibitScale-0.05f:exhibitScale; break;
        case '0': exhibitX=exhibitY=exhibitZ=exhibitRotY=0; exhibitScale=1; break;
        case 'p': case 'P': animateScene = !animateScene; break;
    }
}

void keyboardUp(unsigned char key,int,int) {
    keyDown[key] = false;
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
    glutCreateWindow("Interactive 3D Smart Museum - Commit 2");

    initOpenGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16,timer,0);

    glutMainLoop();
    return 0;
}
