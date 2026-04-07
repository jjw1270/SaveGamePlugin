// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveGameEditorStyle.h"
#include "CommonUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"

TSharedPtr<FSlateStyleSet> FSaveGameEditorStyle::_StyleSet;

#define IMAGE_BRUSH(_relative_path, _size) \
	FSlateImageBrush(_StyleSet->RootToContentDir(_relative_path, TEXT(".png")), _size)

void FSaveGameEditorStyle::Initialize()
{
	if (IsValid(_StyleSet))
		return;

	_StyleSet = MakeShareable(new FSlateStyleSet("SaveGameEditorStyle"));

	TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin("SaveGame");
	_StyleSet->SetContentRoot(plugin->GetBaseDir() / TEXT("Resources"));

	const FVector2D icon40(40.f, 40.f);
	_StyleSet->Set("SaveGame.Clear", new IMAGE_BRUSH("ClearSaveGameData_40", icon40));

	FSlateStyleRegistry::RegisterSlateStyle(*_StyleSet.Get());
}

void FSaveGameEditorStyle::Shutdown()
{
	if (IsInvalid(_StyleSet))
		return;

	FSlateStyleRegistry::UnRegisterSlateStyle(*_StyleSet.Get());
	ensure(_StyleSet.IsUnique());
	_StyleSet.Reset();
}

const ISlateStyle& FSaveGameEditorStyle::Get()
{
	return *_StyleSet.Get();
}

FName FSaveGameEditorStyle::GetStyleSetName()
{
	static FName style_set_name(TEXT("SaveGameEditorStyle"));
	return style_set_name;
}
