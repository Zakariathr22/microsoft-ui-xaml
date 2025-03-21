// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "ContentControl.g.h"

#define __ToolTip_GUID "59251340-a834-4f86-83db-f15ce55c7ea4"

namespace DirectUI
{
    class ToolTip;
    class ToolTipTemplateSettings;
    class UIElement;

    class __declspec(novtable) ToolTipGenerated:
        public DirectUI::ContentControl
        , public ABI::Microsoft::UI::Xaml::Controls::IToolTip
    {
        friend class DirectUI::ToolTip;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.ToolTip");

        BEGIN_INTERFACE_MAP(ToolTipGenerated, DirectUI::ContentControl)
            INTERFACE_ENTRY(ToolTipGenerated, ABI::Microsoft::UI::Xaml::Controls::IToolTip)
        END_INTERFACE_MAP(ToolTipGenerated, DirectUI::ContentControl)

    public:
        ToolTipGenerated();
        ~ToolTipGenerated() override;

        // Event source typedefs.
        typedef CRoutedEventSource<ABI::Microsoft::UI::Xaml::IRoutedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::IRoutedEventArgs> ClosedEventSourceType;
        typedef CRoutedEventSource<ABI::Microsoft::UI::Xaml::IRoutedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::IRoutedEventArgs> OpenedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ToolTip;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ToolTip;
        }

        // Properties.
        IFACEMETHOD(get_HorizontalOffset)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_HorizontalOffset)(DOUBLE value) override;
        IFACEMETHOD(get_IsOpen)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsOpen)(BOOLEAN value) override;
        IFACEMETHOD(get_Placement)(_Out_ ABI::Microsoft::UI::Xaml::Controls::Primitives::PlacementMode* pValue) override;
        IFACEMETHOD(put_Placement)(ABI::Microsoft::UI::Xaml::Controls::Primitives::PlacementMode value) override;
        IFACEMETHOD(get_PlacementRect)(_Out_ ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Rect>** ppValue) override;
        IFACEMETHOD(put_PlacementRect)(ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Rect>* pValue) override;
        IFACEMETHOD(get_PlacementTarget)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue) override;
        IFACEMETHOD(put_PlacementTarget)(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue) override;
        IFACEMETHOD(get_TemplateSettings)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IToolTipTemplateSettings** ppValue) override;
        _Check_return_ HRESULT put_TemplateSettings(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IToolTipTemplateSettings* pValue);
        IFACEMETHOD(get_VerticalOffset)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_VerticalOffset)(DOUBLE value) override;

        // Events.
        _Check_return_ HRESULT GetClosedEventSourceNoRef(_Outptr_ ClosedEventSourceType** ppEventSource);
        IFACEMETHOD(add_Closed)(_In_ ABI::Microsoft::UI::Xaml::IRoutedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_Closed)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetOpenedEventSourceNoRef(_Outptr_ OpenedEventSourceType** ppEventSource);
        IFACEMETHOD(add_Opened)(_In_ ABI::Microsoft::UI::Xaml::IRoutedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_Opened)(EventRegistrationToken token) override;

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        _Check_return_ HRESULT EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo) override;
        _Check_return_ HRESULT EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler) override;

    private:

        // Fields.
    };
}

#include "ToolTip_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) ToolTipFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IToolTipFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IToolTipStatics
    {
        BEGIN_INTERFACE_MAP(ToolTipFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(ToolTipFactory, ABI::Microsoft::UI::Xaml::Controls::IToolTipFactory)
            INTERFACE_ENTRY(ToolTipFactory, ABI::Microsoft::UI::Xaml::Controls::IToolTipStatics)
        END_INTERFACE_MAP(ToolTipFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IToolTip** ppInstance);

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_HorizontalOffsetProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsOpenProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_PlacementProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_PlacementTargetProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_PlacementRectProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_VerticalOffsetProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ToolTip;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
