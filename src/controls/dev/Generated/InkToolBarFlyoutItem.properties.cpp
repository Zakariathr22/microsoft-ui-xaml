// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "InkToolBarFlyoutItem.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithDPFactory(InkToolBarFlyoutItem)
}

#include "InkToolBarFlyoutItem.g.cpp"

GlobalDependencyProperty InkToolBarFlyoutItemProperties::s_IsCheckedProperty{ nullptr };
GlobalDependencyProperty InkToolBarFlyoutItemProperties::s_KindProperty{ nullptr };

InkToolBarFlyoutItemProperties::InkToolBarFlyoutItemProperties()
    : m_checkedEventSource{static_cast<InkToolBarFlyoutItem*>(this)}
    , m_uncheckedEventSource{static_cast<InkToolBarFlyoutItem*>(this)}
{
    EnsureProperties();
}

void InkToolBarFlyoutItemProperties::EnsureProperties()
{
    if (!s_IsCheckedProperty)
    {
        s_IsCheckedProperty =
            InitializeDependencyProperty(
                L"IsChecked",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::InkToolBarFlyoutItem>(),
                false /* isAttached */,
                ValueHelper<bool>::BoxedDefaultValue(),
                nullptr);
    }
    if (!s_KindProperty)
    {
        s_KindProperty =
            InitializeDependencyProperty(
                L"Kind",
                winrt::name_of<winrt::InkToolBarFlyoutItemKind>(),
                winrt::name_of<winrt::InkToolBarFlyoutItem>(),
                false /* isAttached */,
                ValueHelper<winrt::InkToolBarFlyoutItemKind>::BoxedDefaultValue(),
                nullptr);
    }
}

void InkToolBarFlyoutItemProperties::ClearProperties()
{
    s_IsCheckedProperty = nullptr;
    s_KindProperty = nullptr;
}

void InkToolBarFlyoutItemProperties::IsChecked(bool value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<InkToolBarFlyoutItem*>(this)->SetValue(s_IsCheckedProperty, ValueHelper<bool>::BoxValueIfNecessary(value));
    }
}

bool InkToolBarFlyoutItemProperties::IsChecked()
{
    return ValueHelper<bool>::CastOrUnbox(static_cast<InkToolBarFlyoutItem*>(this)->GetValue(s_IsCheckedProperty));
}

void InkToolBarFlyoutItemProperties::Kind(winrt::InkToolBarFlyoutItemKind const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<InkToolBarFlyoutItem*>(this)->SetValue(s_KindProperty, ValueHelper<winrt::InkToolBarFlyoutItemKind>::BoxValueIfNecessary(value));
    }
}

winrt::InkToolBarFlyoutItemKind InkToolBarFlyoutItemProperties::Kind()
{
    return ValueHelper<winrt::InkToolBarFlyoutItemKind>::CastOrUnbox(static_cast<InkToolBarFlyoutItem*>(this)->GetValue(s_KindProperty));
}

winrt::event_token InkToolBarFlyoutItemProperties::Checked(winrt::TypedEventHandler<winrt::InkToolBarFlyoutItem, winrt::IInspectable> const& value)
{
    return m_checkedEventSource.add(value);
}

void InkToolBarFlyoutItemProperties::Checked(winrt::event_token const& token)
{
    m_checkedEventSource.remove(token);
}

winrt::event_token InkToolBarFlyoutItemProperties::Unchecked(winrt::TypedEventHandler<winrt::InkToolBarFlyoutItem, winrt::IInspectable> const& value)
{
    return m_uncheckedEventSource.add(value);
}

void InkToolBarFlyoutItemProperties::Unchecked(winrt::event_token const& token)
{
    m_uncheckedEventSource.remove(token);
}
