/*****************************************************************************\
 * TP CPE, 4ETI, TP synthese d'images
 * --------------
 *
 * Programme principal des appels OpenGL
 \*****************************************************************************/

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#define GLEW_STATIC 1
#include <GL/glew.h>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#define __gl_h_
#include <GLUT/glut.h>
#define MACOSX_COMPATIBILITY GLUT_3_2_CORE_PROFILE
#else
#include <GL/glut.h>
#define MACOSX_COMPATIBILITY 0
#endif

#include "glhelper.hpp"
#include "mat4.hpp"
#include "vec3.hpp"
#include "vec2.hpp"
#include "triangle_index.hpp"
#include "vertex_opengl.hpp"
#include "image.hpp"
#include "mesh.hpp"

/*****************************************************************************\
 * Variables globales
 *
 *
 \*****************************************************************************/
bool victory = false;

bool dino_r = true;
bool dino_g = true;
bool dino_b = true;

bool tir = false;
int portee = 0;

int compt = 0, min = 0, sec = 0;
std::string m,s;

//identifiant du shader
GLuint shader_program_id;
GLuint gui_program_id;


//Matrice de transformation
struct transformation
{
  mat4 rotation;
  vec3 rotation_center;
  vec3 translation;

  transformation():rotation(),rotation_center(),translation(){}
};

//Informations correspondants à un model
struct model
{
  GLuint vao = 0;
  GLuint texture_id=0;
  int nbr_triangle;
  transformation transformation_model;
  vec3 angle = vec3(0.0f,0.0f,0.0f); // angle suivant x, y et z
  // rotation autour de y non implémentée
};

// Text drawing Structure
struct text{
  GLuint vao = 0;              // Vertex array id
  GLuint texture_id = 0;       // Well, texture id...
  std::string value;           // Value of the text to display
  vec2 bottomLeft;
  vec2 topRight;
};

//Liste des modèls
model model_dinosaure;
model model_sol;
model model_dinosaure_r; //AJOUT
model model_dinosaure_g;
model model_dinosaure_b;
model model_missile;

text text_timer;
text text_mission;

//Transformation de la vue (camera)
transformation transformation_view;
float angle_view = 0.0f;

//Matrice de projection
mat4 projection;

void load_texture(const char* filename,GLuint *texture_id,GLuint program_id);

void init_model_1();
void init_model_2();

void init_model_r(); //AJOUT
void init_model_g();
void init_model_b();

void init_model_missile();

void deplacement_auto();
void deplacement_missile();

void deplacement_r();
void deplacement_g();
void deplacement_b();
void contact(model id, char c);

void draw_model(model model_id);

void init_text_timer(text *t);
void draw_texts(text *text_t);
void timer();
void affiche_mission();
void affiche_victoire();

static void init()
{

  // Chargement du shader
  shader_program_id = glhelper::create_program_from_file(
      "shaders/shader.vert",
      "shaders/shader.frag"); CHECK_GL_ERROR();
  glUseProgram(shader_program_id);

  //matrice de projection
  projection = matrice_projection(60.0f*M_PI/180.0f,1.0f,0.01f,100.0f);
  GLint loc_projection = glGetUniformLocation(shader_program_id, "projection"); CHECK_GL_ERROR();
  if (loc_projection == -1) std::cerr << "Pas de variable uniforme : projection" << std::endl;
  glUniformMatrix4fv(loc_projection,1,false,pointeur(projection)); CHECK_GL_ERROR();

  //centre de rotation de la 'camera' (les objets sont centres en z=-2)
  transformation_view.rotation_center = vec3(0.0f,0.0f,-2.0f);

  //activation de la gestion de la profondeur
  glEnable(GL_DEPTH_TEST); CHECK_GL_ERROR();

  // Charge modele 1 sur la carte graphique
  init_model_1();
  // Charge modele 2 sur la carte graphique
  init_model_2();

  init_model_r(); //AJOUT
  init_model_g();
  init_model_b();

  init_model_missile();

  //
  gui_program_id = glhelper::create_program_from_file(
      "shaders/shader_txt.vert",
      "shaders/shader_txt.frag"); CHECK_GL_ERROR();
  glUseProgram(gui_program_id);

  text_timer.value="00:00";
  text_timer.bottomLeft=vec2(-0.25,-0.9);
  text_timer.topRight=vec2(0.25,-0.4);
  init_text_timer(&text_timer);

  text_mission.value = " Red Green Blue";
  text_mission.bottomLeft = vec2(-1.0,0.9);
  text_mission.topRight = vec2(0.0,2.0);
  init_text_timer(&text_mission);
}

//Fonction d'affichage
static void display_callback()
{
  //effacement des couleurs du fond d'ecran
  glClearColor(0.5f, 0.6f, 0.9f, 1.0f); CHECK_GL_ERROR();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK_GL_ERROR();

  //définie le programme à utiliser
  glUseProgram(shader_program_id);

  // Affecte les parametres uniformes de la vue (identique pour tous les modeles de la scene)
  {

    //envoie de la rotation
    GLint loc_rotation_view = glGetUniformLocation(shader_program_id, "rotation_view"); CHECK_GL_ERROR();
    if (loc_rotation_view == -1) std::cerr << "Pas de variable uniforme : rotation_view" << std::endl;
    glUniformMatrix4fv(loc_rotation_view,1,false,pointeur(transformation_view.rotation)); CHECK_GL_ERROR();

    //envoie du centre de rotation
    vec3 cv = transformation_view.rotation_center;
    GLint loc_rotation_center_view = glGetUniformLocation(shader_program_id, "rotation_center_view"); CHECK_GL_ERROR();
    if (loc_rotation_center_view == -1) std::cerr << "Pas de variable uniforme : rotation_center_view" << std::endl;
    glUniform4f(loc_rotation_center_view , cv.x,cv.y,cv.z , 0.0f); CHECK_GL_ERROR();

    //envoie de la translation
    vec3 tv = transformation_view.translation;
    GLint loc_translation_view = glGetUniformLocation(shader_program_id, "translation_view"); CHECK_GL_ERROR();
    if (loc_translation_view == -1) std::cerr << "Pas de variable uniforme : translation_view" << std::endl;
    glUniform4f(loc_translation_view , tv.x,tv.y,tv.z , 0.0f); CHECK_GL_ERROR();
  }

  // Affiche le modele dinosaure
  draw_model(model_dinosaure);
  // Affiche le modele sol
  draw_model(model_sol);

  if (dino_r) {draw_model(model_dinosaure_r);} //AJOUT
  if (dino_g) {draw_model(model_dinosaure_g);}
  if (dino_b) {draw_model(model_dinosaure_b);}

  if (tir) {draw_model(model_missile);}

  
  //
  glUseProgram(gui_program_id);

  glDisable(GL_DEPTH_TEST);
  draw_texts(&text_timer);
  draw_texts(&text_mission);
  glEnable(GL_DEPTH_TEST);
  //Changement de buffer d'affichage pour eviter un effet de scintillement
  glutSwapBuffers();


  //Changement de buffer d'affichage pour eviter un effet de scintillement
  glutSwapBuffers();
}

/*****************************************************************************\
 * keyboard_callback                                                         *
 \*****************************************************************************/
static void keyboard_callback(unsigned char key, int, int)
{
  float d_angle=0.1f;
  float dz=0.25f; //0.125f

  //quitte le programme si on appuie sur les touches 'q', 'Q', ou 'echap'
  //enregistre l'image si on appuie sur la touche 'p'
  switch (key)
  {
    case 'p':
      glhelper::print_screen();
      break;
    case 'q':
    case 'Q':
    case 27:
      exit(0);
      break;

    case 'o':
      if (model_dinosaure.angle.x < 1.1) {
      model_dinosaure.angle.x += d_angle;
      }
      break;
    case 'l':
      if (model_dinosaure.angle.x > -1.9) {
      model_dinosaure.angle.x -= d_angle;
      }
      break;

    case 'k':
      model_dinosaure.angle.y += d_angle;
      break;
    case 'm':
      model_dinosaure.angle.y -= d_angle;
      break;

    case 'n':
      if (fmod(abs(model_dinosaure.angle.x),6.3) < 1.575 || fmod(abs(model_dinosaure.angle.x),6.3) > 4.725) {
        model_dinosaure.transformation_model.translation.z += dz * cos(model_dinosaure.angle.y);
        model_dinosaure.transformation_model.translation.x += dz * sin(model_dinosaure.angle.y);
      }
      else {
        model_dinosaure.transformation_model.translation.z -= dz * cos(model_dinosaure.angle.y);
        model_dinosaure.transformation_model.translation.x -= dz * sin(model_dinosaure.angle.y);
      }
      model_dinosaure.transformation_model.translation.y -= dz * sin(model_dinosaure.angle.x);
      break;
    
    case 'b':
      tir = true;
      break;



    case 's':
      angle_view += d_angle;
      break;
    case 'f':
      angle_view -= d_angle;
      break;


    case 'e':
      transformation_view.translation.z += dz;
      break;
    case 'd':
      transformation_view.translation.z -= dz;
      break;
  }

  /*
  model_dinosaure.transformation_model.rotation = matrice_rotation(model_dinosaure.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_dinosaure.angle.x, 1.0f,0.0f,0.0f);
  transformation_view.rotation = matrice_rotation(angle_view , 0.0f,1.0f,0.0f);
  // Des exemples de camera vous sont données dans le programme 10
  
  // Exemple camera troisieme personne :
  transformation_view.rotation_center = vec3();
  vec3 target = model_dinosaure.transformation_model.translation + vec3(0.,-0.5,-2.);
  vec3 pos_camera = target - model_dinosaure.transformation_model.rotation * vec3(0., -2., 5.) ;
  transformation_view.rotation = matrice_lookat(pos_camera, target, vec3(0., 1., 0.));
  transformation_view.translation = extract_translation(transformation_view.rotation);
  */
}

/*****************************************************************************\
 * special_callback                                                          *
 \*****************************************************************************/
static void special_callback(int key, int,int)
{
  float dL=0.03f;
  switch (key)
  {
    /*
    case GLUT_KEY_UP:
      model_dinosaure.transformation_model.translation.y += dL; //rotation avec la touche du haut
      break;
    case GLUT_KEY_DOWN:
      model_dinosaure.transformation_model.translation.y -= dL; //rotation avec la touche du bas
      break;
    case GLUT_KEY_LEFT:
      model_dinosaure.transformation_model.translation.x -= dL; //rotation avec la touche de gauche
      break;
    case GLUT_KEY_RIGHT:
      model_dinosaure.transformation_model.translation.x += dL; //rotation avec la touche de droite
      break;
    */
  }
}


/*****************************************************************************\
 * timer_callback                                                            *
 \*****************************************************************************/
static void timer_callback(int)
{
  if (victory) {
    affiche_victoire();
  }

  else {
    //demande de rappel de cette fonction dans 25ms
    glutTimerFunc(25, timer_callback, 0);

    timer();

    contact(model_dinosaure_r, 'r');
    contact(model_dinosaure_g, 'g');
    contact(model_dinosaure_b, 'b');
    affiche_mission();

    // Déplacement IA
    deplacement_r();
    deplacement_g();
    deplacement_b();

    // Déplacement automatique
    deplacement_auto();
    deplacement_missile();
  
    if (!(dino_r || dino_g || dino_b)) {victory = true;}
  }

  // Affichage auto
  model_dinosaure.transformation_model.rotation = matrice_rotation(model_dinosaure.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_dinosaure.angle.x, 1.0f,0.0f,0.0f);
  transformation_view.rotation = matrice_rotation(angle_view , 0.0f,1.0f,0.0f);
  
  // Exemple camera troisieme personne :
  transformation_view.rotation_center = vec3();
  vec3 target = model_dinosaure.transformation_model.translation + vec3(0.,-0.5,-2.);  
  vec3 pos_camera = target - model_dinosaure.transformation_model.rotation * vec3(0., -2., 5.) ;
  transformation_view.rotation = matrice_lookat(pos_camera, target, vec3(0., 1., 0.));
  transformation_view.translation = extract_translation(transformation_view.rotation);

  //reactualisation de l'affichage
  glutPostRedisplay();
}

int main(int argc, char** argv)
{
  //**********************************************//
  //Lancement des fonctions principales de GLUT
  //**********************************************//

  //initialisation
  glutInit(&argc, argv);

  //Mode d'affichage (couleur, gestion de profondeur, ...)
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | MACOSX_COMPATIBILITY);

  //Taille de la fenetre a l'ouverture
  glutInitWindowSize(600, 600);

  //Titre de la fenetre
  glutCreateWindow("OpenGL");

  //Fonction de la boucle d'affichage
  glutDisplayFunc(display_callback);

  //Fonction de gestion du clavier
  glutKeyboardFunc(keyboard_callback);

  //Fonction des touches speciales du clavier (fleches directionnelles)
  glutSpecialFunc(special_callback);

  //Fonction d'appel d'affichage en chaine
  glutTimerFunc(25, timer_callback, 0);

  //Option de compatibilité
  glewExperimental = true;

  //Initialisation des fonctions OpenGL
  glewInit();

  //Affiche la version openGL utilisée
  std::cout << "OpenGL: " << (GLchar *)(glGetString(GL_VERSION)) << std::endl;

  //Notre fonction d'initialisation des donnees et chargement des shaders
  init();

  //Lancement de la boucle (infinie) d'affichage de la fenetre
  glutMainLoop();

  //Plus rien n'est execute apres cela
  return 0;
}

void draw_model(model m)
{

  //envoie des parametres uniformes
  {
    GLint loc_rotation_model = glGetUniformLocation(shader_program_id, "rotation_model"); CHECK_GL_ERROR();
    if (loc_rotation_model == -1) std::cerr << "Pas de variable uniforme : rotation_model" << std::endl;
    glUniformMatrix4fv(loc_rotation_model,1,false,pointeur(m.transformation_model.rotation));    CHECK_GL_ERROR();

    vec3 c = m.transformation_model.rotation_center;
    GLint loc_rotation_center_model = glGetUniformLocation(shader_program_id, "rotation_center_model"); CHECK_GL_ERROR();
    if (loc_rotation_center_model == -1) std::cerr << "Pas de variable uniforme : rotation_center_model" << std::endl;
    glUniform4f(loc_rotation_center_model , c.x,c.y,c.z , 0.0f);                                 CHECK_GL_ERROR();

    vec3 t = m.transformation_model.translation;
    GLint loc_translation_model = glGetUniformLocation(shader_program_id, "translation_model"); CHECK_GL_ERROR();
    if (loc_translation_model == -1) std::cerr << "Pas de variable uniforme : translation_model" << std::endl;
    glUniform4f(loc_translation_model , t.x,t.y,t.z , 0.0f);                                     CHECK_GL_ERROR();
  }

  glBindVertexArray(m.vao);CHECK_GL_ERROR();

  //affichage
  {
    glBindTexture(GL_TEXTURE_2D, m.texture_id);                             CHECK_GL_ERROR();
    glDrawElements(GL_TRIANGLES, 3*m.nbr_triangle, GL_UNSIGNED_INT, 0);     CHECK_GL_ERROR();
  }
}

void init_model_1()
{
  // Chargement d'un maillage a partir d'un fichier
  mesh m = load_obj_file("data/stegosaurus.obj");

  // Affecte une transformation sur les sommets du maillage
  float s = 0.2f;
  mat4 transform = mat4(   s, 0.0f, 0.0f, 0.0f,
      0.0f,    s, 0.0f,-0.9f,
      0.0f, 0.0f,   s ,-2.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  apply_deformation(&m,transform);

  // Centre la rotation du modele 1 autour de son centre de gravite approximatif
  model_dinosaure.transformation_model.rotation_center = vec3(0.0f,-0.5f,-2.0f);

  // Calcul automatique des normales du maillage
  update_normals(&m);
  // Les sommets sont affectes a une couleur blanche
  fill_color(&m,vec3(1.0f,1.0f,1.0f));

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_dinosaure.vao);
  glBindVertexArray(model_dinosaure.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo); CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo); CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,m.vertex.size()*sizeof(vertex_opengl),&m.vertex[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi); CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi); CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.connectivity.size()*sizeof(triangle_index),&m.connectivity[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Nombre de triangles de l'objet 1
  model_dinosaure.nbr_triangle = m.connectivity.size();

  // Chargement de la texture
  load_texture("data/stegosaurus.tga",&model_dinosaure.texture_id,shader_program_id);
}

void init_model_2()
{
  //Creation manuelle du model 2

  //coordonnees geometriques des sommets
  vec3 p0=vec3(-100.0f,-0.9f,-100.0f);  // 25.0
  vec3 p1=vec3( 100.0f,-0.9f,-100.0f);
  vec3 p2=vec3( 100.0f,-0.9f, 100.0f);
  vec3 p3=vec3(-100.0f,-0.9f, 100.0f);

  //normales pour chaque sommet
  vec3 n0=vec3(0.0f,1.0f,0.0f);
  vec3 n1=n0;
  vec3 n2=n0;
  vec3 n3=n0;

  //couleur pour chaque sommet
  vec3 c0=vec3(1.0f,1.0f,1.0f);
  vec3 c1=c0;
  vec3 c2=c0;
  vec3 c3=c0;

  //texture du sommet
  vec2 t0=vec2(0.0f,0.0f);
  vec2 t1=vec2(1.0f,0.0f);
  vec2 t2=vec2(1.0f,1.0f);
  vec2 t3=vec2(0.0f,1.0f);

  vertex_opengl v0=vertex_opengl(p0,n0,c0,t0);
  vertex_opengl v1=vertex_opengl(p1,n1,c1,t1);
  vertex_opengl v2=vertex_opengl(p2,n2,c2,t2);
  vertex_opengl v3=vertex_opengl(p3,n3,c3,t3);

  //tableau entrelacant coordonnees-normales
  vertex_opengl geometrie[]={v0,v1,v2,v3};

  //indice des triangles
  triangle_index tri0=triangle_index(0,1,2);
  triangle_index tri1=triangle_index(0,2,3);
  triangle_index index[]={tri0,tri1};
  model_sol.nbr_triangle = 2;

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_sol.vao);
  glBindVertexArray(model_sol.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo);                                             CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo);                                CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,sizeof(geometrie),geometrie,GL_STATIC_DRAW);  CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi);                                            CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi);                       CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(index),index,GL_STATIC_DRAW);  CHECK_GL_ERROR();

  // Chargement de la texture
  load_texture("data/grass.tga",&model_sol.texture_id,shader_program_id);

}

void load_texture(const char* filename,GLuint *texture_id,GLuint program_id)
{
  // Chargement d'une texture (seul les textures tga sont supportes)
  Image  *image = image_load_tga(filename);
  if (image) //verification que l'image est bien chargee
  {

    //Creation d'un identifiant pour la texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); CHECK_GL_ERROR();
    glGenTextures(1, texture_id); CHECK_GL_ERROR();

    //Selection de la texture courante a partir de son identifiant
    glBindTexture(GL_TEXTURE_2D, *texture_id); CHECK_GL_ERROR();

    //Parametres de la texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); CHECK_GL_ERROR();

    //Envoie de l'image en memoire video
    if(image->type==IMAGE_TYPE_RGB){ //image RGB
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data); CHECK_GL_ERROR();}
    else if(image->type==IMAGE_TYPE_RGBA){ //image RGBA
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data); CHECK_GL_ERROR();}
    else{
      std::cout<<"Image type not handled"<<std::endl;}

    delete image;
  }
  else
  {
    std::cerr<<"Erreur chargement de l'image, etes-vous dans le bon repertoire?"<<std::endl;
    abort();
  }

  GLint loc_texture = glGetUniformLocation(program_id, "texture"); CHECK_GL_ERROR();
  if (loc_texture == -1) std::cerr << "Pas de variable uniforme : texture" << std::endl;
  glUniform1i (loc_texture,0); CHECK_GL_ERROR();
}




// AJOUT
void init_model_r()
{
  // Chargement d'un maillage a partir d'un fichier
  mesh m = load_obj_file("data/stegosaurus.obj");

  // Affecte une transformation sur les sommets du maillage
  float s = 0.2f;
  mat4 transform = mat4(   s, 0.0f, 0.0f, 0.0f,
      0.0f,    s, 0.0f,-0.9f,
      0.0f, 0.0f,   s ,-2.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  apply_deformation(&m,transform);

  // Centre la rotation du modele 1 autour de son centre de gravite approximatif
  model_dinosaure_r.transformation_model.rotation_center = vec3(0.0f,-0.5f,-2.0f);

  // Calcul automatique des normales du maillage
  update_normals(&m);
  // Les sommets sont affectes a une couleur blanche
  fill_color(&m,vec3(1.0f,0.0f,0.0f));

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_dinosaure_r.vao);
  glBindVertexArray(model_dinosaure_r.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo); CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo); CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,m.vertex.size()*sizeof(vertex_opengl),&m.vertex[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi); CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi); CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.connectivity.size()*sizeof(triangle_index),&m.connectivity[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Nombre de triangles de l'objet 1
  model_dinosaure_r.nbr_triangle = m.connectivity.size();

  // Chargement de la texture
  load_texture("data/stegosaurus.tga",&model_dinosaure_r.texture_id,shader_program_id);

  model_dinosaure_r.transformation_model.translation.z = 20;
  model_dinosaure_r.transformation_model.translation.x = 5;
}

void init_model_g()
{
  // Chargement d'un maillage a partir d'un fichier
  mesh m = load_obj_file("data/stegosaurus.obj");

  // Affecte une transformation sur les sommets du maillage
  float s = 0.2f;
  mat4 transform = mat4(   s, 0.0f, 0.0f, 0.0f,
      0.0f,    s, 0.0f,-0.9f,
      0.0f, 0.0f,   s ,-2.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  apply_deformation(&m,transform);

  // Centre la rotation du modele 1 autour de son centre de gravite approximatif
  model_dinosaure_g.transformation_model.rotation_center = vec3(0.0f,-0.5f,-2.0f);

  // Calcul automatique des normales du maillage
  update_normals(&m);
  // Les sommets sont affectes a une couleur blanche
  fill_color(&m,vec3(0.0f,1.0f,0.0f));

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_dinosaure_g.vao);
  glBindVertexArray(model_dinosaure_g.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo); CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo); CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,m.vertex.size()*sizeof(vertex_opengl),&m.vertex[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi); CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi); CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.connectivity.size()*sizeof(triangle_index),&m.connectivity[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Nombre de triangles de l'objet 1
  model_dinosaure_g.nbr_triangle = m.connectivity.size();

  // Chargement de la texture
  load_texture("data/stegosaurus.tga",&model_dinosaure_g.texture_id,shader_program_id);

  model_dinosaure_g.transformation_model.translation.z = 20;
  model_dinosaure_g.transformation_model.translation.y = 2;
}

void init_model_b()
{
  // Chargement d'un maillage a partir d'un fichier
  mesh m = load_obj_file("data/stegosaurus.obj");

  // Affecte une transformation sur les sommets du maillage
  float s = 0.2f;
  mat4 transform = mat4(   s, 0.0f, 0.0f, 0.0f,
      0.0f,    s, 0.0f,-0.9f,
      0.0f, 0.0f,   s ,-2.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  apply_deformation(&m,transform);

  // Centre la rotation du modele 1 autour de son centre de gravite approximatif
  model_dinosaure_b.transformation_model.rotation_center = vec3(0.0f,-0.5f,-2.0f);

  // Calcul automatique des normales du maillage
  update_normals(&m);
  // Les sommets sont affectes a une couleur blanche
  fill_color(&m,vec3(0.0f,0.0f,1.0f));

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_dinosaure_b.vao);
  glBindVertexArray(model_dinosaure_b.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo); CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo); CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,m.vertex.size()*sizeof(vertex_opengl),&m.vertex[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi); CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi); CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.connectivity.size()*sizeof(triangle_index),&m.connectivity[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Nombre de triangles de l'objet 1
  model_dinosaure_b.nbr_triangle = m.connectivity.size();

  // Chargement de la texture
  load_texture("data/stegosaurus.tga",&model_dinosaure_b.texture_id,shader_program_id);

  model_dinosaure_b.transformation_model.translation.z = 20;
  model_dinosaure_b.transformation_model.translation.x = -5;
}

void deplacement_r()
{
  float d_angle=0.2f;
  float dz=0.05f;

  int movx = rand() % 3 - 1;
  int movy = rand() % 3 - 1;

  model_dinosaure_r.angle.x += movx*d_angle;
  model_dinosaure_r.angle.y += movy*d_angle;

  if (fmod(abs(model_dinosaure_r.angle.x),6.3) < 1.575 || fmod(abs(model_dinosaure_r.angle.x),6.3) > 4.725) {
    model_dinosaure_r.transformation_model.translation.z += dz * cos(model_dinosaure_r.angle.y);
    model_dinosaure_r.transformation_model.translation.x += dz * sin(model_dinosaure_r.angle.y);
  }
  else {
    model_dinosaure_r.transformation_model.translation.z -= dz * cos(model_dinosaure_r.angle.y);
    model_dinosaure_r.transformation_model.translation.x -= dz * sin(model_dinosaure_r.angle.y);
  }
  model_dinosaure_r.transformation_model.translation.y -= dz * sin(model_dinosaure_r.angle.x);
  if (model_dinosaure_r.transformation_model.translation.y < 0) {
    model_dinosaure_r.transformation_model.translation.y += dz;
    model_dinosaure_r.angle.x = 0;
  }

  model_dinosaure_r.transformation_model.rotation = matrice_rotation(model_dinosaure_r.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_dinosaure_r.angle.x, 1.0f,0.0f,0.0f);
  glutPostRedisplay();
}

void deplacement_g()
{
  float d_angle=0.2f;
  float dz=0.05f;

  int movx = rand() % 3 - 1;
  int movy = rand() % 3 - 1;

  model_dinosaure_g.angle.x += movx*d_angle;
  model_dinosaure_g.angle.y += movy*d_angle;

  if (fmod(abs(model_dinosaure_g.angle.x),6.3) < 1.575 || fmod(abs(model_dinosaure_g.angle.x),6.3) > 4.725) {
    model_dinosaure_g.transformation_model.translation.z += dz * cos(model_dinosaure_g.angle.y);
    model_dinosaure_g.transformation_model.translation.x += dz * sin(model_dinosaure_g.angle.y);
  }
  else {
    model_dinosaure_g.transformation_model.translation.z -= dz * cos(model_dinosaure_g.angle.y);
    model_dinosaure_g.transformation_model.translation.x -= dz * sin(model_dinosaure_g.angle.y);
  }
  model_dinosaure_g.transformation_model.translation.y -= dz * sin(model_dinosaure_g.angle.x);
  if (model_dinosaure_g.transformation_model.translation.y < 0) {
    model_dinosaure_g.transformation_model.translation.y += dz;
    model_dinosaure_g.angle.x = 0;
  }

  model_dinosaure_g.transformation_model.rotation = matrice_rotation(model_dinosaure_g.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_dinosaure_g.angle.x, 1.0f,0.0f,0.0f);
  glutPostRedisplay();
}

void deplacement_b()
{
  float d_angle=0.2f;
  float dz=0.05f;

  int movx = rand() % 3 - 1;
  int movy = rand() % 3 - 1;

  model_dinosaure_b.angle.x += movx*d_angle;
  model_dinosaure_b.angle.y += movy*d_angle;

  if (fmod(abs(model_dinosaure_b.angle.x),6.3) < 1.575 || fmod(abs(model_dinosaure_b.angle.x),6.3) > 4.725) {
    model_dinosaure_b.transformation_model.translation.z += dz * cos(model_dinosaure_b.angle.y);
    model_dinosaure_b.transformation_model.translation.x += dz * sin(model_dinosaure_b.angle.y);
  }
  else {
    model_dinosaure_b.transformation_model.translation.z -= dz * cos(model_dinosaure_b.angle.y);
    model_dinosaure_b.transformation_model.translation.x -= dz * sin(model_dinosaure_b.angle.y);
  }
  model_dinosaure_b.transformation_model.translation.y -= dz * sin(model_dinosaure_b.angle.x);
  if (model_dinosaure_b.transformation_model.translation.y < 0) {
    model_dinosaure_b.transformation_model.translation.y += dz;
    model_dinosaure_b.angle.x = 0;
  }

  model_dinosaure_b.transformation_model.rotation = matrice_rotation(model_dinosaure_b.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_dinosaure_b.angle.x, 1.0f,0.0f,0.0f);
  glutPostRedisplay();
}

void contact(model id, char c)
{
  int x = model_missile.transformation_model.translation.x;
  int y = model_missile.transformation_model.translation.y;
  int z = model_missile.transformation_model.translation.z;
  int x2 = id.transformation_model.translation.x;
  int y2 = id.transformation_model.translation.y;
  int z2 = id.transformation_model.translation.z;
  int d = 1.5;
  if (tir && x2 < x+d && x2 > x-d && y2 < y+d && y2 > y-d && z2 < z+d && z2 > z-d)
  {
    if (c == 'r') {
      dino_r = false;
      tir = false;
    }
    else if (c == 'g') {
      dino_g = false;
      tir = false;
    }
    else {
      dino_b = false;
      tir = false;
    }
  }
}


void init_model_missile()
{
  // Chargement d'un maillage a partir d'un fichier
  mesh m = load_obj_file("data/stegosaurus.obj");

  // Affecte une transformation sur les sommets du maillage
  float s = 0.07f;
  mat4 transform = mat4(   s, 0.0f, 0.0f, 0.0f,
      0.0f,    s, 0.0f,-0.9f,
      0.0f, 0.0f,   s ,-2.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  apply_deformation(&m,transform);

  // Centre la rotation du modele 1 autour de son centre de gravite approximatif
  model_missile.transformation_model.rotation_center = vec3(0.0f,-0.5f,-2.0f);

  // Calcul automatique des normales du maillage
  update_normals(&m);
  // Les sommets sont affectes a une couleur blanche
  fill_color(&m,vec3(0.0f,0.0f,0.0f));

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_missile.vao);
  glBindVertexArray(model_missile.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo); CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo); CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,m.vertex.size()*sizeof(vertex_opengl),&m.vertex[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi); CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi); CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.connectivity.size()*sizeof(triangle_index),&m.connectivity[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Nombre de triangles de l'objet 1
  model_missile.nbr_triangle = m.connectivity.size();

  // Chargement de la texture
  load_texture("data/stegosaurus.tga",&model_missile.texture_id,shader_program_id);

  model_missile.transformation_model.translation = model_dinosaure.transformation_model.translation;
}

void deplacement_auto() {
  float dz=0.075f;
  
  if (fmod(abs(model_dinosaure.angle.x),6.3) < 1.575 || fmod(abs(model_dinosaure.angle.x),6.3) > 4.725) {
    model_dinosaure.transformation_model.translation.z += dz * cos(model_dinosaure.angle.y);
    model_dinosaure.transformation_model.translation.x += dz * sin(model_dinosaure.angle.y);
  }
  else {
    model_dinosaure.transformation_model.translation.z -= dz * cos(model_dinosaure.angle.y);
    model_dinosaure.transformation_model.translation.x -= dz * sin(model_dinosaure.angle.y);
  }
  model_dinosaure.transformation_model.translation.y -= dz * sin(model_dinosaure.angle.x);
  if (model_dinosaure.transformation_model.translation.y < 0) {
    model_dinosaure.transformation_model.translation.y += dz;
    model_dinosaure.angle.x = 0;
  }
}

void deplacement_missile() {
  float dz=0.4f;
  int d = 50;
  if (tir) {
    if (fmod(abs(model_missile.angle.x),6.3) < 1.575 || fmod(abs(model_missile.angle.x),6.3) > 4.725) {
      model_missile.transformation_model.translation.z += dz * cos(model_missile.angle.y);
      model_missile.transformation_model.translation.x += dz * sin(model_missile.angle.y);
    }
    else {
      model_missile.transformation_model.translation.z -= dz * cos(model_missile.angle.y);
      model_missile.transformation_model.translation.x -= dz * sin(model_missile.angle.y);
    }
    model_missile.transformation_model.translation.y -= dz * sin(model_missile.angle.x);
    portee += 1;
    if (portee > d) {
      portee = 0;
      tir = false;
    }
    if (model_missile.transformation_model.translation.y < 0) {
      portee = 0;
      tir = false;
    }
  }
  else {
    model_missile.transformation_model.translation = model_dinosaure.transformation_model.translation ;
    model_missile.angle = model_dinosaure.angle;
  }
  model_missile.transformation_model.rotation = matrice_rotation(model_missile.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_missile.angle.x, 1.0f,0.0f,0.0f);
}


//
void init_text_timer(text *t) {

  vec3 p0=vec3( 0.0f, 0.0f, 0.0f);
  vec3 p1=vec3( 0.0f, 1.0f, 0.0f);
  vec3 p2=vec3( 1.0f, 1.0f, 0.0f);
  vec3 p3=vec3( 1.0f, 0.0f, 0.0f);

  vec3 geometrie[4] = {p0, p1, p2, p3}; 
  triangle_index index[2] = { triangle_index(0, 1, 2), triangle_index(0, 2, 3)};

  glGenVertexArrays(1, &(t->vao));                                              CHECK_GL_ERROR();
  glBindVertexArray(t->vao);                                                  CHECK_GL_ERROR();

  GLuint vbo;
  glGenBuffers(1, &vbo);                                                       CHECK_GL_ERROR();
  glBindBuffer(GL_ARRAY_BUFFER,vbo);                                          CHECK_GL_ERROR();
  glBufferData(GL_ARRAY_BUFFER,sizeof(geometrie),geometrie,GL_STATIC_DRAW);   CHECK_GL_ERROR();

  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); CHECK_GL_ERROR();

  GLuint vboi;
  glGenBuffers(1,&vboi);                                                      CHECK_GL_ERROR();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi);                                 CHECK_GL_ERROR();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(index),index,GL_STATIC_DRAW);   CHECK_GL_ERROR();

  load_texture("data/font.tga",&t->texture_id,gui_program_id);
}

void draw_texts(text *text_t) {
  vec2 size = (text_t->topRight - text_t->bottomLeft) / float(text_t->value.size());
  // std::cout << size << " " << text_t->bottomLeft << std::endl;
  
  //Send uniforma parameters
  
  GLint loc_size = glGetUniformLocation(gui_program_id, "size"); //CHECK_GL_ERROR();
  if (loc_size == -1) std::cerr << "Pas de variable uniforme : size" << std::endl;
  glUniform2f(loc_size,size.x, size.y);    //CHECK_GL_ERROR();

  glBindVertexArray(text_t->vao);//CHECK_GL_ERROR();
  
  for(long unsigned int i = 0; i < text_t->value.size(); ++i)
  {
    GLint loc_start = glGetUniformLocation(gui_program_id, "start"); //CHECK_GL_ERROR();
    if (loc_start == -1) std::cerr << "Pas de variable uniforme : start" << std::endl;
    glUniform2f(loc_start,text_t->bottomLeft.x+i*size.x, text_t->bottomLeft.y);   // CHECK_GL_ERROR();

    GLint loc_char = glGetUniformLocation(gui_program_id, "c");// CHECK_GL_ERROR();
    if (loc_char == -1) std::cerr << "Pas de variable uniforme : c" << std::endl;
    glUniform1i(loc_char, (int)text_t->value[i]);   // CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, text_t->texture_id);                   //    CHECK_GL_ERROR();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);               //     CHECK_GL_ERROR();
  }
}

void timer() {
  if (compt == 40){
    compt = 0;
    sec++;
    if (sec == 60){
      sec = 0;
      min++;
    }
    s = std::to_string(sec);
    m = std::to_string(min);
    if (min < 10) {
      m = "0" + m;
    }
    if (sec < 10) {
    	s = "0" + s;
    }
    text_timer.value = m+":"+s;
  }
  compt++;
}

void affiche_mission() {
  std::string mission = " ";
  int n = 0;
  if (dino_r) {
    mission += "Red ";
    n++;
  }
  if (dino_g) {
    mission += "Green ";
    n++;
  }
  if (dino_b) {
    mission += "Blue ";
    n++;
  }
  if (n==2) {
    text_mission.topRight = vec2(-0.25,1.75);
  }
  if (n==1) {
    text_mission.topRight = vec2(-0.55,1.5);
  }
  text_mission.value = mission;
}

void affiche_victoire() {
  text_mission.value = "VICTORY";
  text_mission.bottomLeft = vec2(-0.4,0.4);
  text_mission.topRight = vec2(0.4,1.3);
}