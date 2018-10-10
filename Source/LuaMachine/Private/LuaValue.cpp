// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaValue.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Editor/PropertyEditor/Public/IDetailChildrenBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Input/STextComboBox.h"
#include "Modules/ModuleManager.h"


void FLuaValueCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);

	TSharedPtr<IPropertyHandle> LuaValueTypeProperty = PropertyHandle->GetChildHandle(FName(TEXT("Type")));

	HeaderRow.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		].ValueContent()[
			LuaValueTypeProperty->CreatePropertyValueWidget()
		];
}

EVisibility FLuaValueCustomization::IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle, ELuaValueType WantedValueType)
{
	TSharedPtr<IPropertyHandle> LuaValueTypeProperty = PropertyHandle->GetChildHandle(FName(TEXT("Type")));
	if (!LuaValueTypeProperty.IsValid())
		return EVisibility::Hidden;

	uint8 ValueType;
	LuaValueTypeProperty->GetValue(ValueType);

	if ((ELuaValueType)ValueType == WantedValueType)
		return EVisibility::Visible;

	return EVisibility::Hidden;
}

void FLuaValueCustomization::LuaFunctionChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectionType, TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject *> Objects;
	PropertyHandle->GetOuterObjects(Objects);

	if (Objects.Num() != 1)
		return;

	UFunction* FoundFunction = Objects[0]->FindFunction(FName(*(*Value.Get())));
	if (FoundFunction)
	{
		PropertyHandle->SetValue(FoundFunction->GetName());
	}
}

void FLuaValueCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TArray<UObject *> Objects;
	PropertyHandle->GetOuterObjects(Objects);

	if (Objects.Num() != 1)
		return;

	TSharedPtr<IPropertyHandle> LuaValueBoolProperty = PropertyHandle->GetChildHandle(FName(TEXT("Bool")));
	IDetailPropertyRow& PropertyBoolRow = Builder.AddProperty(LuaValueBoolProperty.ToSharedRef());
	PropertyBoolRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuaValueCustomization::IsPropertyVisible, PropertyHandle, ELuaValueType::Bool)));

	TSharedPtr<IPropertyHandle> LuaValueStringProperty = PropertyHandle->GetChildHandle(FName(TEXT("String")));
	IDetailPropertyRow& PropertyStringRow = Builder.AddProperty(LuaValueStringProperty.ToSharedRef());
	PropertyStringRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuaValueCustomization::IsPropertyVisible, PropertyHandle, ELuaValueType::String)));

	TSharedPtr<IPropertyHandle> LuaValueIntegerProperty = PropertyHandle->GetChildHandle(FName(TEXT("Integer")));
	IDetailPropertyRow& PropertyIntegerRow = Builder.AddProperty(LuaValueIntegerProperty.ToSharedRef());
	PropertyIntegerRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuaValueCustomization::IsPropertyVisible, PropertyHandle, ELuaValueType::Integer)));

	TSharedPtr<IPropertyHandle> LuaValueNumberProperty = PropertyHandle->GetChildHandle(FName(TEXT("Number")));
	IDetailPropertyRow& PropertyNumberRow = Builder.AddProperty(LuaValueNumberProperty.ToSharedRef());
	PropertyNumberRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuaValueCustomization::IsPropertyVisible, PropertyHandle, ELuaValueType::Number)));

	TSharedPtr<IPropertyHandle> LuaValueObjectProperty = PropertyHandle->GetChildHandle(FName(TEXT("ObjectPath")));
	IDetailPropertyRow& PropertyObjectRow = Builder.AddProperty(LuaValueObjectProperty.ToSharedRef());
	PropertyObjectRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuaValueCustomization::IsPropertyVisible, PropertyHandle, ELuaValueType::Object)));

	TSharedPtr<IPropertyHandle> LuaValueFunctionProperty = PropertyHandle->GetChildHandle(FName(TEXT("FunctionName")));

	FString CurrentFunctionName;
	LuaValueFunctionProperty->GetValue(CurrentFunctionName);

	TSharedPtr<FString> CurrentSelectedFunction;

	for (TFieldIterator<UFunction> Funcs(Objects[0]->GetClass()); Funcs; ++Funcs)
	{
		UFunction *Function = *Funcs;

		if (!Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Public))
			continue;

		bool bIsValid = true;
		for (TFieldIterator<UProperty> FArgs(Function); FArgs && FArgs->PropertyFlags & CPF_Parm; ++FArgs)
		{
			UProperty *Prop = *FArgs;
			UStructProperty* LuaProp = Cast<UStructProperty>(Prop);
			if (!LuaProp)
			{
				bIsValid = false;
				break;
			}
			if (LuaProp->Struct != FLuaValue::StaticStruct())
			{
				bIsValid = false;
				break;
			}
		}

		if (!bIsValid)
			continue;

		TSharedPtr<FString> FunctionNameSP = MakeShareable(new FString(Function->GetName()));

		if (CurrentFunctionName == Function->GetName())
		{
			CurrentSelectedFunction = FunctionNameSP;
		}

		ValidLuaFunctions.Add(FunctionNameSP);
	}


	Builder.AddChildContent(FText::FromString(TEXT("Function"))).ValueContent()[
		SNew(STextComboBox).OptionsSource(&ValidLuaFunctions).OnSelectionChanged_Raw(this, &FLuaValueCustomization::LuaFunctionChanged, LuaValueFunctionProperty.ToSharedRef()).InitiallySelectedItem(CurrentSelectedFunction)
	].Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateRaw(this, &FLuaValueCustomization::IsPropertyVisible, PropertyHandle, ELuaValueType::Function)));
}
