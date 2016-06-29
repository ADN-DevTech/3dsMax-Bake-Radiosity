// (C) Copyright 2011 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software
// in object code form for any purpose and without fee is hereby
// granted, provided that the above copyright notice appears in
// all copies and that both that copyright notice and the limited
// warranty and restricted rights notice below appear in all
// supporting documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK,
// INC. DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL
// BE UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is
// subject to restrictions set forth in FAR 52.227-19 (Commercial
// Computer Software - Restricted Rights) and DFAR 252.227-7013(c)
// (1)(ii)(Rights in Technical Data and Computer Software), as
// applicable.
//


#include "BakeRadiosity.h"

#define BakeRadiosity_CLASS_ID	Class_ID(0x833514a7, 0x9c5b982f)


class BakeRadiosity : public UtilityObj 
{
public:
   void DoIt (void);
   void SetKeepOrginal (bool a_bCheck);

   virtual void BeginEditParams(Interface *ip,IUtil *iu);
	virtual void EndEditParams(Interface *ip,IUtil *iu);
		
	virtual void Init(HWND hWnd);
	virtual void Destroy(HWND hWnd);

	virtual void DeleteThis() { }		

	//Constructor/Destructor
	BakeRadiosity();
	virtual ~BakeRadiosity();

	// Singleton access
	static BakeRadiosity* GetInstance() { 
		static BakeRadiosity theBakeRadiosity;
		return &theBakeRadiosity; 
	}

private:
	bool keepOrgFlag;
   bool CreateNewMesh (INode *, Mesh *, Matrix3);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, 
                                    UINT msg, 
                                    WPARAM wParam,
                                    LPARAM lParam);

	HWND			hPanel;
	IUtil			*iu;
	Interface	*ip;
};


class BakeRadiosityClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 	{ return BakeRadiosity::GetInstance(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return UTILITY_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return BakeRadiosity_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("BakeRadiosity"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2 *GetBakeRadiosityDesc()
{ 
	static BakeRadiosityClassDesc bakeRadiosityDesc;
	return &bakeRadiosityDesc; 
}

INT_PTR CALLBACK BakeRadiosity::DlgProc(HWND hWnd, 
                                        UINT msg, 
                                        WPARAM wParam, 
                                        LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			BakeRadiosity::GetInstance()->Init(hWnd);
			break;

		case WM_DESTROY:
			BakeRadiosity::GetInstance()->Destroy(hWnd);
			break;

		case WM_COMMAND:
			if(LOWORD(wParam) == IDC_BUTTON){
            // Launches this feature
				BakeRadiosity::GetInstance()->DoIt();
			} else
			if(LOWORD(wParam) == IDC_CHECK){
            // Sets the flag for delete original nodes
            if(::IsDlgButtonChecked(hWnd, IDC_CHECK) == BST_CHECKED){
					BakeRadiosity::GetInstance()->SetKeepOrginal(true);
				} else {
					BakeRadiosity::GetInstance()->SetKeepOrginal(false);
				}
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			BakeRadiosity::GetInstance()->ip->RollupMouseMessage(hWnd,
                                                               msg,
                                                               wParam,
                                                               lParam); 
			break;

		default:
			return 0;
	}
	return 1;
}



//--- BakeRadiosity -------------------------------------------------------
BakeRadiosity::BakeRadiosity()
{
   keepOrgFlag = false;
	iu = NULL;
	ip = NULL;	
	hPanel = NULL;
}

BakeRadiosity::~BakeRadiosity()
{

}

void BakeRadiosity::BeginEditParams(Interface* ip,IUtil* iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		DlgProc,
		GetString(IDS_PARAMS),
		0);
}
	
void BakeRadiosity::EndEditParams(Interface* ip, IUtil* /* iu*/) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
}

void BakeRadiosity::Init(HWND /*hWnd*/)
{

}

void BakeRadiosity::Destroy(HWND /*hWnd*/)
{

}

bool BakeRadiosity::CreateNewMesh (INode *orgNode, 
                                    Mesh *orgMesh, 
                                    Matrix3 orgMtx)
{
   if((orgNode == NULL)||(orgMesh == NULL)){
	   DebugPrint(_T("Mesh error\n"));
      return false;
   }
   // Creates an instance of a registered class.
   Object *newObj = (Object*)(ip->CreateInstance(
                                       GEOMOBJECT_CLASS_ID,
                                       Class_ID(TRIOBJ_CLASS_ID, 0)));
   if(newObj == NULL){
	   DebugPrint(_T("CreateInstance error\n"));
      return false;
   }
   // Creates a new node in the scene with the given object. 
   INode *newNode = ip->CreateObjectNode(newObj);
   if(newNode == NULL){
	   DebugPrint(_T("CreateObjectNode error\n"));
      return false;
   }
   // Sets the name of the node. 
   if(keepOrgFlag != true){
      newNode->SetName(orgNode->GetName());
   } else {
      TSTR newName;
      newName.printf(_T("%s_BAKED"), orgNode->GetName());
      newNode->SetName(newName);
   }
   // Sets the renderer material used by the node.
   newNode->SetMtl(orgNode->GetMtl());
   // Returns a reference to the mesh data member of new TriObject.
   TriObject *newTriObj = (TriObject *)newObj;
   Mesh &newMesh = newTriObj->GetMesh();
   // Returns the number of vertices from original mesh. 
   int nbVert = orgMesh->getNumVerts();
   // Sets the number of geometric vertices in the new mesh.
   newMesh.setNumVerts(nbVert);
   // The loop will continue until handling all vertices...
   for(int i=0; i<nbVert; i++) { 
      newMesh.verts[i] = orgMtx * orgMesh->verts[i];//Set new vertices
   }
   // Returns the number of faces in the original mesh.
   int nbFace = orgMesh->getNumFaces();
   // Sets the number of faces in the new mesh 
   // and previous faces are discarded.
   newMesh.setNumFaces(nbFace, FALSE);
   // The loop will continue until handling all faces...
   for(int i=0; i<nbFace; i++){ // Set new faces and Material id
      newMesh.faces[i] = orgMesh->faces[i];
      newMesh.faces[i].setMatID(orgMesh->faces[i].getMatID());
   }
   // Makes a complete copy of the specified channels 
   // of the original Mesh object into new Mesh.
   newMesh.DeepCopy(orgMesh, CNVERT_CHANNELS);

   return true;
}

void BakeRadiosity::DoIt(void)
{
   DebugPrint(_T("Plug-in Start\n"));

   RadiosityInterface *ri;
   RadiosityEffect *re;
   RadiosityMesh *rm;
   INode *node;
   Tab<INode *> selNodes;
   int nbSel;

   // Gets core interface
   ri = static_cast<RadiosityInterface*>(
            GetCOREInterface(RADIOSITY_INTERFACE));
   if(ri == NULL){
	   DebugPrint(_T("RadiosityInterface error\n"));
      return;
   }
   // Returns a pointer to the currently active Advanced Lighting plug-in.
   if((re = ri->GetRadiosity()) == NULL){
      DebugPrint(_T("RadiosityEffect error\n"));
      return;
   }
   // Gets Radiosity Mesh
   if((rm = GetRadiosityMesh(re)) == NULL){
      DebugPrint(_T("RadiosityMesh error\n"));
      return;
   }
   // Resets the container for INode pointers.
    selNodes.ZeroCount();
   // Returns the number of nodes in the selection set.  
   nbSel = ip->GetSelNodeCount();
   // The loop will continue until handling all seletecd nodes...
   for(int i=0; i<nbSel; i++){
      // Returns a pointer to the 'i-th' node in the selection set.
      if((node = ip->GetSelNode(i)) == NULL){
         continue;
      }
      // Gets the mesh for a node.
      Mesh *mesh = NULL;
      if(!rm->GetMesh(node, mesh)||(mesh == NULL)){
	      DebugPrint(_T("GetMesh error\n"));
         continue;
      }
      // Gets the TM for the node.
      // TM is the mesh to world space transform.
      Matrix3 mtx(1);
      if(!rm->GetMeshTM(node, mtx)){
	      DebugPrint(_T("GetMeshTM error\n"));
         continue;
      }
      // Creates a new mesh
      if(!CreateNewMesh(node, mesh, mtx)){
	      DebugPrint(_T("CreateMesh error\n"));
         continue;
      }
      // Appends a INode pointer to the container
      selNodes.Append(1, &node);
   }
   // Deletes original nodes if the flag is set.
   if(keepOrgFlag != true){
      nbSel = selNodes.Count();
      // The loop will continue until handling all appended nodes...
      for(int i=0; i<nbSel; i++){
         if(selNodes[i] == NULL){
            continue;
         }
         // Removes it from the hierarchy, and handle undo. 
         // The position of any children of this node are kept the same.
         selNodes[i]->Delete(ip->GetTime(), TRUE);
      }
   }

   DebugPrint(_T("Plug-in End\n"));
}

// Sets the flag for delete original nodes
void BakeRadiosity::SetKeepOrginal (bool value)
{
   keepOrgFlag = value;
}

