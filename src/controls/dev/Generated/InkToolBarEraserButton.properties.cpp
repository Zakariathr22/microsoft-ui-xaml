// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "InkToolBarEraserButton.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithDPFactory(InkToolBarEraserButton)
}

#include "InkToolBarEraserButton.g.cpp"

GlobalDependencyProperty InkToolBarEraserButtonProperties::s_ArePrecisionErasersVisibleProperty{ nullptr };
GlobalDependencyProperty InkToolBarEraserButtonProperties::s_IsClearAllVisibleProperty{ nullptr };
GlobalDependencyProperty InkToolBarEraserButtonProperties::s_IsStrokeEraserVisibleProperty{ nullptr };
GlobalDependencyProperty InkToolBarEraserButtonProperties::s_SelectedEraserProperty{ nullptr };

InkToolBarEraserButtonProperties::InkToolBarEraserButtonProperties()
{
    EnsureProperties();
}

void InkToolBarEraserButtonProperties::EnsureProperties()
{
    InkToolBarToolButton::EnsureProperties();
    if (!s_ArePrecisionErasersVisibleProperty)
    {
        s_ArePrecisionErasersVisibleProperty =
            InitializeDependencyProperty(
                L"ArePrecisionErasersVisible",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::InkToolBarEraserButton>(),
                false /* isAttached */,
                ValueHelper<bool>::BoxedDefaultValue(),
                nullptr);
    }
    if (!s_IsClearAllVisibleProperty)
    {
        s_IsClearAllVisibleProperty =
            InitializeDependencyProperty(
                L"IsClearAllVisible",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::InkToolBarEraserButton>(),
                false /* isAttached */,
                ValueHelper<bool>::BoxedDefaultValue(),
                nullptr);
    }
    if (!s_IsStrokeEraserVisibleProperty)
    {
        s_IsStrokeEraserVisibleProperty =
            InitializeDependencyProperty(
                L"IsStrokeEraserVisible",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::InkToolBarEraserButton>(),
                false /* isAttached */,
                ValueHelper<bool>::BoxedDefaultValue(),
                nullptr);
    }
    if (!s_SelectedEraserProperty)
    {
        s_SelectedEraserProperty =
            InitializeDependencyProperty(
                L"SelectedEraser",
                winrt::name_of<winrt::InkToolBarEraserKind>(),
                winrt::name_of<winrt::InkToolBarEraserButton>(),
                false /* isAttached */,
                ValueHelper<winrt::InkToolBarEraserKind>::BoxedDefaultValue(),
                nullptr);
    }
}

void InkToolBarEraserButtonProperties::ClearProperties()
{
    s_ArePrecisionErasersVisibleProperty = nullptr;
    s_IsClearAllVisibleProperty = nullptr;
    s_IsStrokeEraserVisibleProperty = nullptr;
    s_SelectedEraserProperty = nullptr;
    InkToolBarToolButton::ClearProperties();
}

void InkToolBarEraserButtonProperties::ArePrecisionErasersVisible(bool value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<InkToolBarEraserButton*>(this)->SetValue(s_ArePrecisionErasersVisibleProperty, ValueHelper<bool>::BoxValueIfNecessary(value));
    }
}

bool InkToolBarEraserButtonProperties::ArePrecisionErasersVisible()
{
    return ValueHelper<bool>::CastOrUnbox(static_cast<InkToolBarEraserButton*>(this)->GetValue(s_ArePrecisionErasersVisibleProperty));
}

void InkToolBarEraserButtonProperties::IsClearAllVisible(bool value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<InkToolBarEraserButton*>(this)->SetValue(s_IsClearAllVisibleProperty, ValueHelper<bool>::BoxValueIfNecessary(value));
    }
}

bool InkToolBarEraserButtonProperties::IsClearAllVisible()
{
    return ValueHelper<bool>::CastOrUnbox(static_cast<InkToolBarEraserButton*>(this)->GetValue(s_IsClearAllVisibleProperty));
}

void InkToolBarEraserButtonProperties::IsStrokeEraserVisible(bool value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<InkToolBarEraserButton*>(this)->SetValue(s_IsStrokeEraserVisibleProperty, ValueHelper<bool>::BoxValueIfNecessary(value));
    }
}

bool InkToolBarEraserButtonProperties::IsStrokeEraserVisible()
{
    return ValueHelper<bool>::CastOrUnbox(static_cast<InkToolBarEraserButton*>(this)->GetValue(s_IsStrokeEraserVisibleProperty));
}

void InkToolBarEraserButtonProperties::SelectedEraser(winrt::InkToolBarEraserKind const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<InkToolBarEraserButton*>(this)->SetValue(s_SelectedEraserProperty, ValueHelper<winrt::InkToolBarEraserKind>::BoxValueIfNecessary(value));
    }
}

winrt::InkToolBarEraserKind InkToolBarEraserButtonProperties::SelectedEraser()
{
    return ValueHelper<winrt::InkToolBarEraserKind>::CastOrUnbox(static_cast<InkToolBarEraserButton*>(this)->GetValue(s_SelectedEraserProperty));
}
