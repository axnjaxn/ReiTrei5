#include "parser.h"
#include "mesh.h"

/*
 * Parser
 */

Real Parser::parseNumber() {
  Token token = ts.getToken();

  if (token == "-") return -parseNumber();
  else if (token == "+") return parseNumber();
  else if (token == "(") {
    Real r = parseReal();
    ts.expectToken(")");
    return r;
  }
  
  //Validate token
  bool haspt = 0;
  for (int i = 0; token[i]; i++)
    if (token[i] == '.') {
      if (!haspt) haspt = 1;
      else throw ParseError("_Real_", token, ts.lineNumber());
    }
    else if (!isdigit(token[i])) throw ParseError("_Real_", token, ts.lineNumber());

  Real r;
  sscanf(token.c_str(), "%lf", &r);
  return r;
}

Real Parser::parseTerm() {
  Real r = parseNumber();
  for (;;) {
    Token token = ts.getToken();
    if (token == "*" && ts.peekToken() != "<") r = r * parseNumber();
    else if (token == "/") r = r / parseNumber();
    else {
      ts.ungetToken(token);
      break;
    }
  }
  return r;
}

Real Parser::parseReal() {
  Real r = parseTerm();
  for (;;) {
    Token token = ts.getToken();
    if (token == "+") r = r + parseTerm();
    else if (token == "-") r = r - parseTerm();
    else {
      ts.ungetToken(token);
      break;
    }
  }
  return r;
}

Real Parser::parseAngle() {
  return PI * parseReal() / 180.0;
}

Vect3 Parser::parseTriplet() {
  ts.expectToken("<");
  Vect3 v;
  v[0] = parseReal();
  ts.expectToken(",");
  v[1] = parseReal();
  ts.expectToken(",");
  v[2] = parseReal();
  ts.expectToken(">");
  return v;
}

Vect3 Parser::parseVectorTerm() {
  Real coef = 1.0;
  Vect3 v;
  if (ts.peekToken() != "<") {
    coef = parseReal();
    ts.expectToken("*");
  }
  v = parseTriplet();
  for (;;) {
    Token token = ts.peekToken();
    if (token == "*") {
      ts.getToken();
      coef = coef * parseNumber();
    }
    else if (token == "/") {
      ts.getToken();
      coef = coef / parseNumber();
    }
    else break;
  }
  return coef * v;
}

Vect3 Parser::parseVector() {
  Vect3 v = parseVectorTerm();
  for (;;) {
    Token token = ts.getToken();
    if (token == "+") v = v + parseVectorTerm();
    else if (token == "-") v = v - parseVectorTerm();
    else {
      ts.ungetToken(token);
      break;
    }
  }
  return v;
}

void Parser::parseModifiers(Object* obj) {
  while (parsedModifier(obj) || parsedMaterial(&obj->material));
}

Box* Parser::parseBox() {
  if (ts.peekToken() != "Box") return NULL;
  else ts.getToken();

  ts.expectToken("{");
  Vect3 upper = parseVector();
  Vect3 lower = parseVector();
  Box* box = new Box();
  box->scale((upper - lower) / 2);
  box->translate((upper + lower) / 2);
  parseModifiers(box);
  ts.expectToken("}");
  return box;
}

Sphere* Parser::parseSphere() {
  if (ts.peekToken() != "Sphere") return NULL;
  else ts.getToken();

  ts.expectToken("{");
  Vect3 center = parseVector();
  Real radius = parseReal();
  Sphere* sphere = new Sphere();
  sphere->scale(Vect3(radius, radius, radius));
  sphere->translate(center);
  parseModifiers(sphere);
  ts.expectToken("}");
  return sphere;
}

Cone* Parser::parseCone() {
  if (ts.peekToken() != "Cone") return NULL;
  else ts.getToken();

  ts.expectToken("{");
  Cone* cone = new Cone();
  do {
    if (ts.peekToken() == "nocaps") {
      ts.getToken();
      cone->has_caps = 0;
    }
  } while (parsedModifier(cone) || parsedMaterial(&cone->material));
  ts.expectToken("}");
  return cone;
}

Plane* Parser::parsePlane() {
  if (ts.peekToken() != "Plane") return NULL;
  else ts.getToken();

  ts.expectToken("{");
  Plane* plane = new Plane();
  plane->A = parseVector();
  plane->N = parseVector();
  parseModifiers(plane);
  ts.expectToken("}");
  return plane;
}

Triangle* Parser::parseTriangle() {
  if (ts.peekToken() != "Triangle") return NULL;
  else ts.getToken();

  ts.expectToken("{");
  Vect3 a = parseVector();
  Vect3 b = parseVector();
  Vect3 c = parseVector();

  Triangle* tri;
  if (ts.peekToken() == "<") {
    //This triangle has normal vectors too!
    Vect3 n0 = parseVector();
    Vect3 n1 = parseVector();
    Vect3 n2 = parseVector();    
    tri = new InterpTriangle(a, b, c, n0, n1, n2);//uf test this
  }
  else tri = new Triangle(a, b, c);

  parseModifiers(tri);
  ts.expectToken("}");
  return tri;
}

Object* Parser::parseShape() {
  Object* obj;
  if (obj = parseBox()) return obj;
  else if (obj = parseSphere()) return obj;
  else if (obj = parseCone()) return obj;
  else if (obj = parsePlane()) return obj;
  else if (obj = parseTriangle()) return obj;
  else return NULL;
}

Light* Parser::parseLight() {
  if (ts.peekToken() != "Light") return NULL;
  else ts.getToken(); 

  ts.expectToken("{");
  Light* light = new Light();
  light->position = parseVector();
  light->color = parseVector();
  while (ts.peekToken() != "}") {
    Token token = ts.getToken();
    if (token == "intensity") light->intensity = parseReal();
    else if (token == "radius") light->radius = parseReal();    
    else if (token == "falloff") light->falloff = 1;
    else throw ParseError("_LightProperty_", token, ts.lineNumber());
  }
  ts.expectToken("}");
  return light;
}

bool Parser::parsedMacro() {
  if (ts.peekToken() != "Define") return 0;
  else ts.getToken();

  Macro macro(ts.getToken()); 
  ts.expectToken("{");
  
  int braces_count = 1;
  for (Token value = ts.getToken(); value != "}"; value = ts.getToken()) {
    if (value == "{") braces_count++;
    else if (value == "}") {if (--braces_count == 0) break;}
    else if (ts.eof()) throw ParseError("}", "_EOF_", ts.lineNumber());

    macro.addToken(value);
  }

  ts.addMacro(macro);
  return 1;
}

bool Parser::parsedMaterialProperty(Material* mat) {
  Token token = ts.getToken();
  if (token == "invisible") mat->invisible = 1;
  else if (token == "shadowless") mat->shadowless = 1;
  else if (token == "twosided") mat->twosided = 1;
  else if (token == "ambient") mat->ambient = parseVector();
  else if (token == "diffuse") mat->diffuse = parseVector();
  else if (token == "reflective") mat->reflective = parseVector();
  else if (token == "refractive") {
    mat->refractive = parseVector();
    mat->refractive_index = parseReal();
  }
  else if (token == "specular") mat->specular = parseReal();
  else if (token == "shininess") mat->shininess = parseReal();
  else {
    ts.ungetToken(token);
    return 0;
  }

  return 1;
}

bool Parser::parsedMaterial(Material* mat) {
  if (ts.peekToken() != "material") return 0;
  else ts.getToken();

  ts.expectToken("{");
  while (parsedMaterialProperty(mat));
  ts.expectToken("}");
  return 1;
}

bool Parser::parsedModifier(Modifier* mod) {
  Token token = ts.getToken();

  if (token == "translate") 
    mod->translate(parseVector());
  else if (token == "scale") 
    mod->scale(parseVector());
  else if (token == "rotate") {
    token = ts.getToken();
    if (token == "x") mod->xrotate(-parseAngle());
    else if (token == "y") mod->yrotate(-parseAngle());
    else if (token == "z") mod->zrotate(-parseAngle());
    else throw ParseError("_Axis_", token, ts.lineNumber());
  }
  else if (token == "pinch") {
    Real m = parseReal();
    Real n = parseReal();
    mod->pinch(m, n);
  }
  else {
    ts.ungetToken(token);
    return 0;
  }  

  return 1;
}

bool Parser::parsedShape(Scene* scene) {
  Object* obj = parseShape();
  if (obj) scene->addObject(obj);
  return obj;
}

bool Parser::parsedSetModifiers(ObjectSet* set) {
  Token token = ts.getToken();

  if (token == "unit") {
    Vect3 lower, upper;
    set->getBounds(&lower, &upper);

    Real scale;

    token = ts.getToken();
    if (token == "x") scale = (upper[0] - lower[0]) / 2;
    else if (token == "y") scale = (upper[1] - lower[1]) / 2;
    else if (token == "z") scale = (upper[2] - lower[2]) / 2;
    else throw ParseError("_Axis_", token, ts.lineNumber());

    for (int i = 0; i < set->count(); i++)
      (*set)[i]->scale(Vect3(1, 1, 1) / scale);
  }
  else if (token == "center") {
    Vect3 lower, upper;
    set->getBounds(&lower, &upper);

    Vect3 center = (lower + upper) / 2;
    for (int i = 0; i < set->count(); i++)
      (*set)[i]->translate(-center);
  }
  else {
    ts.ungetToken(token);
    return 0;
  }

  return 1;
}

bool Parser::parsedMesh(Scene* scene) {
  if (ts.peekToken() != "OBJ") return 0;
  else ts.getToken();
  
  ts.expectToken("{");
  ObjectSet set = readOBJ(ts.getToken());
  Modifier mod;
  Material mat;
  do {
    if (parsedModifier(&mod)) {
      for (int i = 0; i < set.count(); i++)
	set[i]->applyModifier(mod);
      continue;
    }
  } while (parsedSetModifiers(&set) || parsedMaterial(&mat));
  for (int i = 0; i < set.count(); i++) {
    set[i]->material = mat;
    scene->addObject(set[i]);
  }
  ts.expectToken("}");
  return 1;
}

bool Parser::parsedLight(Scene* scene) {
  Light* light = parseLight();
  if (light) scene->addLight(light);
  return light;
}

bool Parser::parsedBG(Scene* scene) {
  if (ts.peekToken() != "BGColor") return 0;
  
  ts.getToken();
  scene->bgcolor = parseVector();
  return 1;
}

bool Parser::parsedCamera(Scene* scene) {
  if (ts.peekToken() != "Camera") return 0;
  else ts.getToken();

  ts.expectToken("{");
  parseModifiers(&scene->camera); 
  ts.expectToken("}");
  return 1;
}

bool Parser::parsedSceneItem(Scene* scene) {
  return (parsedMacro()
	  || parsedShape(scene) 
	  || parsedMesh(scene)
	  || parsedLight(scene)
	  || parsedBG(scene)
	  || parsedCamera(scene));
}

void Parser::setConstant(const std::string& name, const std::string&  value) {
  Macro macro(name);
  macro.addToken(value);
  ts.addMacro(macro);
}

void Parser::parseInto(const char* filename, Scene* scene) {
  if (ts.open(filename)) {
    throw ParseError("Couldn't open scene file.");
  }
  else {
    while (!ts.eof() && (ts.peekToken() != "EOF")) 
      if (!parsedSceneItem(scene)) {
	throw ParseError("_SceneItem_", ts.getToken(), ts.lineNumber());
      }
    ts.close();
  }
}
