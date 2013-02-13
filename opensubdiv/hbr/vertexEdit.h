//
//     Copyright (C) Pixar. All rights reserved.
//
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
//
#ifndef HBRVERTEXEDIT_H
#define HBRVERTEXEDIT_H

#include <algorithm>
#include "../hbr/hierarchicalEdit.h"

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrVertexEdit;

template <class T>
std::ostream& operator<<(std::ostream& out, const HbrVertexEdit<T>& path) {
    out << "vertex path = (" << path.faceid << ' ';
    for (int i = 0; i < path.nsubfaces; ++i) {
        out << static_cast<int>(path.subfaces[i]) << ' ';
    }
    return out << static_cast<int>(path.vertexid) << "), edit = (" << path.edit[0] << ',' << path.edit[1] << ',' << path.edit[2] << ')';
}

template <class T>
class HbrVertexEdit : public HbrHierarchicalEdit<T> {

public:

    HbrVertexEdit(int _faceid, int _nsubfaces, unsigned char *_subfaces, unsigned char _vertexid, int _index, int _width, bool _isP, typename HbrHierarchicalEdit<T>::Operation _op, float *_edit)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), vertexid(_vertexid), index(_index), width(_width), isP(_isP), op(_op) {
        edit = new float[width];
        memcpy(edit, _edit, width * sizeof(float));
    }

    HbrVertexEdit(int _faceid, int _nsubfaces, int *_subfaces, int _vertexid, int _index, int _width, bool _isP, typename HbrHierarchicalEdit<T>::Operation _op, float *_edit)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), vertexid(static_cast<unsigned char>(_vertexid)), index(_index), width(_width), isP(_isP), op(_op) {
        edit = new float[width];
        memcpy(edit, _edit, width * sizeof(float));
    }

    virtual ~HbrVertexEdit() {
        delete[] edit;
    }

    // Return the vertex id (the last element in the path)
    unsigned char GetVertexID() const { return vertexid; }

    friend std::ostream& operator<< <T> (std::ostream& out, const HbrVertexEdit<T>& path);

    // Return index of variable this edit applies to
    int GetIndex() const { return index; }

    // Return width of the variable
    int GetWidth() const { return width; }

    // Get the numerical value of the edit
    const float* GetEdit() const { return edit; }

    // Get the type of operation
    typename HbrHierarchicalEdit<T>::Operation GetOperation() const { return op; }

    virtual void ApplyEditToFace(HbrFace<T>* face) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth()) {
            // Tags the vertex as being edited; it'll figure out what to
            // when GuaranteeNeighbor is called
            face->GetVertex(vertexid)->SetVertexEdit();
        }
        // In any event, mark the face as having a vertex edit (which
        // may only be applied on subfaces)
        face->MarkVertexEdits();
    }

    virtual void ApplyEditToVertex(HbrFace<T>* face, HbrVertex<T>* vertex) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth() &&
                face->GetVertex(vertexid) == vertex) {
            vertex->GetData().ApplyVertexEdit(*const_cast<const HbrVertexEdit<T>*>(this));
        }
    }

#ifdef PRMAN
    virtual void ApplyToBound(struct bbox& bbox, RtMatrix *mx) const {
        if (isP) {
            struct xyz p = *(struct xyz*)edit;
            if (mx)
                MxTransformByMatrix(&p, &p, *mx, 1);
            if (op == HbrHierarchicalEdit<T>::Set) {
                bbox.min.x = std::min(bbox.min.x, p.x);
                bbox.min.y = std::min(bbox.min.y, p.y);
                bbox.min.z = std::min(bbox.min.z, p.z);
                bbox.max.x = std::max(bbox.max.x, p.x);
                bbox.max.y = std::max(bbox.max.y, p.y);
                bbox.max.z = std::max(bbox.max.z, p.z);
            } else if (op == HbrHierarchicalEdit<T>::Add ||
                       op == HbrHierarchicalEdit<T>::Subtract) {
                bbox.min.x -= fabsf(p.x);
                bbox.min.y -= fabsf(p.y);
                bbox.min.z -= fabsf(p.z);
                bbox.max.x += fabsf(p.x);
                bbox.max.y += fabsf(p.y);
                bbox.max.z += fabsf(p.z);
            }
        }
    }
#endif

private:
    const unsigned char vertexid;
    int index;
    int width;
    unsigned isP:1;
    typename HbrHierarchicalEdit<T>::Operation op;
    float* edit;
};

template <class T>
class HbrMovingVertexEdit : public HbrHierarchicalEdit<T> {

public:

    HbrMovingVertexEdit(int _faceid, int _nsubfaces, unsigned char *_subfaces, unsigned char _vertexid, int _index, int _width, bool _isP, typename HbrHierarchicalEdit<T>::Operation _op, float *_edit)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), vertexid(_vertexid), index(_index), width(_width), isP(_isP), op(_op) {
        edit = new float[width * 2];
        memcpy(edit, _edit, 2 * width * sizeof(float));
    }

    HbrMovingVertexEdit(int _faceid, int _nsubfaces, int *_subfaces, int _vertexid, int _index, int _width, bool _isP, typename HbrHierarchicalEdit<T>::Operation _op, float *_edit)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), vertexid(_vertexid), index(_index), width(_width), isP(_isP), op(_op) {
        edit = new float[width * 2];
        memcpy(edit, _edit, 2 * width * sizeof(float));
    }

    virtual ~HbrMovingVertexEdit() {
        delete[] edit;
    }

    // Return the vertex id (the last element in the path)
    unsigned char GetVertexID() const { return vertexid; }

    friend std::ostream& operator<< <T> (std::ostream& out, const HbrVertexEdit<T>& path);

    // Return index of variable this edit applies to
    int GetIndex() const { return index; }

    // Return width of the variable
    int GetWidth() const { return width; }

    // Get the numerical value of the edit
    const float* GetEdit() const { return edit; }

    // Get the type of operation
    typename HbrHierarchicalEdit<T>::Operation GetOperation() const { return op; }

    virtual void ApplyEditToFace(HbrFace<T>* face) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth()) {
            // Tags the vertex as being edited; it'll figure out what to
            // when GuaranteeNeighbor is called
            face->GetVertex(vertexid)->SetVertexEdit();
        }
        // In any event, mark the face as having a vertex edit (which
        // may only be applied on subfaces)
        face->MarkVertexEdits();
    }

    virtual void ApplyEditToVertex(HbrFace<T>* face, HbrVertex<T>* vertex) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth() &&
                face->GetVertex(vertexid) == vertex) {
            vertex->GetData().ApplyMovingVertexEdit(*const_cast<const HbrMovingVertexEdit<T>*>(this));
        }
    }

#ifdef PRMAN
    virtual void ApplyToBound(struct bbox& bbox, RtMatrix *mx) const {
        if (isP) {
            struct xyz p1 = *(struct xyz*)edit;
            struct xyz p2 = *(struct xyz*)&edit[3];
            if (mx) {
                MxTransformByMatrix(&p1, &p1, *mx, 1);
                MxTransformByMatrix(&p2, &p2, *mx, 1);
            }
            if (op == HbrVertexEdit<T>::Set) {
                bbox.min.x = std::min(std::min(bbox.min.x, p1.x), p2.x);
                bbox.min.y = std::min(std::min(bbox.min.y, p1.y), p2.y);
                bbox.min.z = std::min(std::min(bbox.min.z, p1.z), p2.z);
                bbox.max.x = std::max(std::max(bbox.max.x, p1.x), p2.x);
                bbox.max.y = std::max(std::max(bbox.max.y, p1.y), p2.y);
                bbox.max.z = std::max(std::max(bbox.max.z, p1.z), p2.z);
            } else if (op == HbrVertexEdit<T>::Add ||
                       op == HbrVertexEdit<T>::Subtract) {
                float maxx = std::max(fabsf(p1.x), fabsf(p2.x));
                float maxy = std::max(fabsf(p1.y), fabsf(p2.y));
                float maxz = std::max(fabsf(p1.z), fabsf(p2.z));
                bbox.min.x -= maxx;
                bbox.min.y -= maxy;
                bbox.min.z -= maxz;
                bbox.max.x += maxx;
                bbox.max.y += maxy;
                bbox.max.z += maxz;
            }
        }
    }
#endif

private:
    const unsigned char vertexid;
    int index;
    int width;
    unsigned isP:1;
    typename HbrHierarchicalEdit<T>::Operation op;
    float* edit;
};


} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* HBRVERTEXEDIT_H */
