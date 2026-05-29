#pragma once

enum class DepthStencilType{

	Default = 0,					/// Depth test + write
	ReadOnly,						/// Depth test only
	Disable,						/// Nodepth
	Greater,						/// Reverse-z
	NumberOfDepthStencilStates

};

