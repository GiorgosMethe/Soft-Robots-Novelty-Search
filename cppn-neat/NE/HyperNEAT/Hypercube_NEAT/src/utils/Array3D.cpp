/*
Source code from:
Cheney, Nick, Robert MacCurdy, Jeff Clune, and Hod Lipson.
"Unshackling evolution: evolving soft robots with multiple materials and a powerful generative encoding."
In Proceeding of the fifteenth annual conference on Genetic and evolutionary computation conference, pp. 167-174. ACM, 2013.
*/

#include "Array3D.h"

CArray3Df::CArray3Df(void)
{
	pData = NULL; //dynamic array
	XSize = 0;
	YSize = 0;
	ZSize = 0;
	FullSize = 0;
}

CArray3Df::CArray3Df(int x, int y, int z)
{
	pData = NULL; //dynamic array
	IniSpace(x, y, z);
}


CArray3Df::~CArray3Df(void)
{
	DeleteSpace();
}

CArray3Df::CArray3Df(const CArray3Df& rArray) //copy constructor
{
	XSize = rArray.XSize;
	YSize = rArray.YSize;
	ZSize = rArray.ZSize;
	FullSize = XSize*YSize*ZSize;

	pData = new float[FullSize];
	for(int i=0; i<FullSize; i++)
		pData[i] = rArray[i];
}

CArray3Df& CArray3Df::operator=(const CArray3Df& rArray) //overload "=" 
{
	DeleteSpace();

	XSize = rArray.XSize;
	YSize = rArray.YSize;
	ZSize = rArray.ZSize;
	FullSize = XSize*YSize*ZSize;

	pData = new float[FullSize];
	for(int i=0; i<FullSize; i++)
		pData[i] = rArray[i];

	return *this;
}

void CArray3Df::IniSpace(int x, int y, int z, float IniVal)
{
	if (pData != NULL)
		DeleteSpace();

	XSize = x;
	YSize = y;
	ZSize = z;
	FullSize = x*y*z;

	pData = new float[FullSize];

	ResetSpace(IniVal);
}

void CArray3Df::ResetSpace(float IniVal)
{
	if (pData != NULL)
		for(int i=0; i<FullSize; i++)
			pData[i] = IniVal;

}

void CArray3Df::DeleteSpace()
{
	if (pData != NULL) delete[] pData;
	pData = NULL;
}

float CArray3Df::GetMaxValue() //returns the maximum value anywhere in the array.
{
	float MaxVal = -9e9f;
	for(int i=0; i<FullSize; i++) if (pData[i] > MaxVal) MaxVal = pData[i];
	return MaxVal;
}

//Index calculating!
int CArray3Df::GetIndex(int x, int y, int z) //returns the index in the frequency space or -1 if invalid
{
	if (x<0 || x >= XSize || y<0 || y >= YSize || z<0 || z >= ZSize) //if this XYZ is out of the area
		return -1;
	else
		return x + XSize*y + XSize*YSize*z;
}

bool CArray3Df::GetXYZ(int* x, int* y, int* z, int Index) //returns the index in the frequency space or -1 if invalid
{
	if (Index<0 || Index > XSize*YSize*ZSize){ *x = -1; *y = -1;	*z = -1; return false;}
	else {
		*z = (int)((float)Index / (XSize*YSize)); //calculate the indices in x, y, z directions
		*y = (int)(((float)Index - *z*XSize*YSize)/XSize);
		*x = Index - *z*XSize*YSize - *y*XSize;
		return true;
	}
}
