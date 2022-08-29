export module Precomputed;
import <vector>;
import aux;
namespace precomputed
{
	using namespace aux;

	export std::vector<unsigned int> genXRay(unsigned int i, int r, int f)
	{
		std::vector<unsigned int> xray = {};
		if (i >= 64 || i < 0)
			return xray;
		for (int k = 1; isIndex(i, k * r, k * f); ++k)
		{
			xray.push_back(index2index(i, k * r, k * f));
		}
		return xray;
	}

	std::vector<std::vector<unsigned int>> genXRaysForSquare(unsigned int i)
	{
		return { genXRay(i, 1, 0), genXRay(i, 0, 1), genXRay(i, -1, 0), genXRay(i, 0, -1),
				genXRay(i, 1, 1), genXRay(i, -1, 1), genXRay(i, -1, -1), genXRay(i, 1, -1) };
	}
	
	using xraylist = std::vector<std::vector<unsigned int>>;
	export std::vector<xraylist> initXRays()
	{
		std::vector<xraylist> ml = {};
		for (int i = 0; i != 64; ++i)
			ml.push_back(genXRaysForSquare(i));
		return ml;
	}

	export std::vector<xraylist> xrays = {};

	export std::vector<unsigned int>& getXRay(unsigned int i, int r, int f)
	{
		if (r == 1 && f == 0)
			return xrays[i][0];
		if (r == 0 && f == 1)
			return xrays[i][1];
		if (r == -1 && f == 0)
			return xrays[i][2];
		if (r == 0 && f == -1)
			return xrays[i][3];
		if (r == 1 && f == 1)
			return xrays[i][4];
		if (r == -1 && f == 1)
			return xrays[i][5];
		if (r == -1 && f == -1)
			return xrays[i][6];
		if (r == 1 && f == -1)
			return xrays[i][7];
		return xrays[0][0];
	}
}