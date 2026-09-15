#include "Precompiled.h"
#include "Geometry/ObjLoader.h"

void ObjLoader::LoadObj(const char *filename) {
    ObjGroup curGroup{};
    curGroup.f_start = 0;
    groupList.Append(curGroup);

    byte *data;
    int size = BE1::fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return;
    }

    BE1::Vec3 v;
    BE1::Vec3 vn;
    BE1::Vec2 vt;
    ObjFace f;

    BE1::Lexer lexer((const char *)data, size, BE1::va("ObjLoader::LoadObj(\'%s\')", filename), BE1::Lexer::Flag::IgnoreComments);
    BE1::Str token;

    while (lexer.ReadToken(&token, true)) {
        if (token.IsEmpty()) {
            break;
        }

        if (token == "#") {
            lexer.SkipRestOfLine();
        } else if (token == "mtllib") {
            BE1::Str mtlname;
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                mtlname += token;
            }
            //std::string s = filename;
            //std::string basedir = s.substr(0, s.find_last_of("\\/") + 1);
            //LoadMtl(basedir.append(mtlname).c_str());
            BE1::Str basedir = filename;
            basedir.StripFileName();
            LoadMtl(basedir + PATHSEPERATOR_STR + mtlname);
        } else if (token == "usemtl") {
            lexer.ReadToken(&token, false);
            auto &previous = groupList[groupList.Count() - 1];
            previous.f_end = fList.Count();
            if (previous.f_start == previous.f_end) {
                previous.mtl_name = token;
            } else {
                ObjGroup group{};
                group.mtl_name = token;
                group.f_start = fList.Count();
                groupList.Append(group);
            }
        } else if (token == "v") {
            v.y = -lexer.ParseNumber();
            v.z = lexer.ParseNumber();
            v.x = -lexer.ParseNumber();
            vList.Append(v);
        } else if (token == "vt") {
            vt.x = lexer.ParseNumber();
            // Convert OBJ's bottom-left UV origin to the image's top-left origin.
            vt.y = 1.0f - lexer.ParseNumber();
            vtList.Append(vt);
        } else if (token == "vn") {
            vn.y = -lexer.ParseNumber();
            vn.z = lexer.ParseNumber();
            vn.x = -lexer.ParseNumber();
            vn.Normalize();
            vnList.Append(vn);
        } else if (token == "f") {
            int firstFace = (int)fList.Count();
            for (int i = 0; lexer.ReadToken(&token, false); i++) {
                if (token.IsEmpty()) {
                    break;
                }

                lexer.UnreadToken(&token);

                f.v_index = lexer.ParseInt() - 1;
                f.t_index = -1;
                f.n_index = -1;

                lexer.ReadToken(&token, false);
                if (token == "/") {
                    lexer.ReadToken(&token, false);
                    if (token == "/") {
                        f.n_index = lexer.ParseInt() - 1;
                    } else {
                        lexer.UnreadToken(&token);
                        f.t_index = lexer.ParseInt() - 1;

                        lexer.ReadToken(&token, false);
                        if (token == "/") {
                            f.n_index = lexer.ParseInt() - 1;
                        } else {
                            lexer.UnreadToken(&token);
                        }
                    }
                } else {
                    lexer.UnreadToken(&token);
                }

                if (i < 3) {
                    fList.Append(f);
                } else {
                    ObjFace last = fList[fList.Count() - 1];
                    ObjFace first = fList[firstFace];

                    fList.Append(first);
                    fList.Append(last);
                    fList.Append(f);
                }
            }
        } else if (token == "s") { // smoothing face
            lexer.SkipRestOfLine();
        } else {
            lexer.SkipRestOfLine();
        }
    }

    if (groupList.Count() == 0) {
        ObjGroup group;
        group.mtl_name = "";
        group.f_start = 0;
        groupList.Append(group);
    }

    groupList[groupList.Count() - 1].f_end = (int)fList.Count();

    BE1::fileSystem.FreeFile(data);
}

void ObjLoader::LoadMtl(const char *filename) {
    byte *data;
    int size = BE1::fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return;
    }

    ObjMtl mtl;
    mtl.Clear();
    ObjMtl *curMtl = &mtl;

    BE1::Lexer lexer((const char *)data, size, BE1::va("ObjLoader::LoadMtl(\'%s\')", filename), BE1::Lexer::Flag::IgnoreComments);
    BE1::Str token;

    while (lexer.ReadToken(&token, true)) {
        if (token.IsEmpty()) {
            break;
        }

        if (token == "#") {
            lexer.SkipRestOfLine();
        } else if (token == "newmtl") {
            lexer.ReadToken(&token, false);
            mtl.Clear();
            ObjMtl &cur = mtlMap[token] = mtl;
            curMtl = &cur;
        } else if (token == "Ns") {
            curMtl->Ns = lexer.ParseNumber();
        } else if (token == "d") {
            curMtl->d = lexer.ParseNumber();
        } else if (token == "Ka") {
            curMtl->Ka.x = lexer.ParseNumber();
            curMtl->Ka.y = lexer.ParseNumber();
            curMtl->Ka.z = lexer.ParseNumber();
        } else if (token == "Kd") {
            curMtl->Kd.x = lexer.ParseNumber();
            curMtl->Kd.y = lexer.ParseNumber();
            curMtl->Kd.z = lexer.ParseNumber();
        } else if (token == "Ks") {
            curMtl->Ks.x = lexer.ParseNumber();
            curMtl->Ks.y = lexer.ParseNumber();
            curMtl->Ks.z = lexer.ParseNumber();
        } else if (token == "Tf") {
            curMtl->Tf.x = lexer.ParseNumber();
            curMtl->Tf.y = lexer.ParseNumber();
            curMtl->Tf.z = lexer.ParseNumber();
        } else if (token == "map_d") {
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                curMtl->map_d += token;
            }
        } else if (token == "map_Ns") {
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                curMtl->map_Ns += token;
            }
        } else if (token == "map_Ka") {
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                curMtl->map_Ka += token;
            }
        } else if (token == "map_Kd") {
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                curMtl->map_Kd += token;
            }
        } else if (token == "map_Ks") {
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                curMtl->map_Ks += token;
            }
        } else if (token == "map_bump") {
            while (lexer.ReadToken(&token, false) && !token.IsEmpty()) {
                curMtl->map_bump += token;
            }
        } else {
            lexer.SkipRestOfLine();
        }
    }

    BE1::fileSystem.FreeFile(data);
}
