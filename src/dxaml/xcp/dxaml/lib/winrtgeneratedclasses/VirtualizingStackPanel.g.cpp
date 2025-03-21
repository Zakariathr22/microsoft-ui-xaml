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

#include "VirtualizingStackPanel.g.h"
#include "CleanUpVirtualizedItemEventArgs.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::VirtualizingStackPanelGenerated::VirtualizingStackPanelGenerated()
{
}

DirectUI::VirtualizingStackPanelGenerated::~VirtualizingStackPanelGenerated()
{
}

HRESULT DirectUI::VirtualizingStackPanelGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::VirtualizingStackPanel)))
    {
        *ppObject = static_cast<DirectUI::VirtualizingStackPanel*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanel)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanel*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanelOverrides)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanelOverrides*>(this);
    }
    else
    {
        RRETURN(DirectUI::OrientedVirtualizingPanel::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::get_AreScrollSnapPointsRegular(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::VirtualizingStackPanel_AreScrollSnapPointsRegular, pValue));
}
IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::put_AreScrollSnapPointsRegular(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::VirtualizingStackPanel_AreScrollSnapPointsRegular, value));
}
IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::get_Orientation(_Out_ ABI::Microsoft::UI::Xaml::Controls::Orientation* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::VirtualizingStackPanel_Orientation, pValue));
}
IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::put_Orientation(ABI::Microsoft::UI::Xaml::Controls::Orientation value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::VirtualizingStackPanel_Orientation, value));
}

// Events.
_Check_return_ HRESULT DirectUI::VirtualizingStackPanelGenerated::GetCleanUpVirtualizedItemEventEventSourceNoRef(_Outptr_ CleanUpVirtualizedItemEventEventSourceType** ppEventSource)
{
    HRESULT hr = S_OK;

    IFC(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::VirtualizingStackPanel_CleanUpVirtualizedItemEvent, reinterpret_cast<IUntypedEventSource**>(ppEventSource)));

    if (*ppEventSource == nullptr)
    {
        IFC(ctl::ComObject<CleanUpVirtualizedItemEventEventSourceType>::CreateInstance(ppEventSource, TRUE /* fDisableLeakChecks */));
        (*ppEventSource)->Initialize(KnownEventIndex::VirtualizingStackPanel_CleanUpVirtualizedItemEvent, this, /* bUseEventManager */ false);
        IFC(StoreEventSource(KnownEventIndex::VirtualizingStackPanel_CleanUpVirtualizedItemEvent, *ppEventSource));

        // The caller doesn't expect a ref, this function ends in "NoRef".  The ref is now owned by the map (inside StoreEventSource)
        ReleaseInterfaceNoNULL(ctl::iunknown_cast(*ppEventSource));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::add_CleanUpVirtualizedItemEvent(_In_ ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    CleanUpVirtualizedItemEventEventSourceType* pEventSource = nullptr;

    IFC(EventAddPreValidation(pValue, ptToken));
    IFC(DefaultStrictApiCheck(this));
    IFC(GetCleanUpVirtualizedItemEventEventSourceNoRef(&pEventSource));
    IFC(pEventSource->AddHandler(pValue));

    ptToken->value = (INT64)pValue;

Cleanup:
    return hr;
}

IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::remove_CleanUpVirtualizedItemEvent(EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    CleanUpVirtualizedItemEventEventSourceType* pEventSource = nullptr;
    ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventHandler* pValue = (ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventHandler*)tToken.value;

    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(GetCleanUpVirtualizedItemEventEventSourceNoRef(&pEventSource));
    IFC(pEventSource->RemoveHandler(pValue));

    if (!pEventSource->HasHandlers())
    {
        IFC(RemoveEventSource(KnownEventIndex::VirtualizingStackPanel_CleanUpVirtualizedItemEvent));
    }

Cleanup:
    RRETURN(hr);
}

// Methods.
IFACEMETHODIMP DirectUI::VirtualizingStackPanelGenerated::OnCleanUpVirtualizedItem(_In_ ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventArgs* pE)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "VirtualizingStackPanel_OnCleanUpVirtualizedItem", 0);
    }
    ARG_NOTNULL(pE, "e");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<VirtualizingStackPanel*>(this)->OnCleanUpVirtualizedItemImpl(pE));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "VirtualizingStackPanel_OnCleanUpVirtualizedItem", hr);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::VirtualizingStackPanelGenerated::OnCleanUpVirtualizedItemProtected(_In_ ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventArgs* pE)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanelOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->OnCleanUpVirtualizedItem(pE));
    }
    else
    {
        IFC(OnCleanUpVirtualizedItem(pE));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::VirtualizingStackPanelGenerated::EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo)
{
    switch (nEventIndex)
    {
    case KnownEventIndex::VirtualizingStackPanel_CleanUpVirtualizedItemEvent:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventHandler> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                CleanUpVirtualizedItemEventEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetCleanUpVirtualizedItemEventEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->AddHandler(spEventHandler.Get(), handledEventsToo));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    default:
        IFC_RETURN(DirectUI::OrientedVirtualizingPanelGenerated::EventAddHandlerByIndex(nEventIndex, pHandler, handledEventsToo));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::VirtualizingStackPanelGenerated::EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler)
{
    switch (nEventIndex)
    {
    case KnownEventIndex::VirtualizingStackPanel_CleanUpVirtualizedItemEvent:
        {
            ctl::ComPtr<ABI::Microsoft::UI::Xaml::Controls::ICleanUpVirtualizedItemEventHandler> spEventHandler;
            IFC_RETURN(IValueBoxer::UnboxValue(pHandler, spEventHandler.ReleaseAndGetAddressOf()));

            if (nullptr != spEventHandler)
            {
                CleanUpVirtualizedItemEventEventSourceType* pEventSource = nullptr;
                IFC_RETURN(GetCleanUpVirtualizedItemEventEventSourceNoRef(&pEventSource));
                IFC_RETURN(pEventSource->RemoveHandler(spEventHandler.Get()));
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        break;
    default:
        IFC_RETURN(DirectUI::OrientedVirtualizingPanelGenerated::EventRemoveHandlerByIndex(nEventIndex, pHandler));
        break;
    }

    return S_OK;
}

HRESULT DirectUI::VirtualizingStackPanelFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanelStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IVirtualizingStackPanelStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.

// Dependency properties.
IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::get_AreScrollSnapPointsRegularProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::VirtualizingStackPanel_AreScrollSnapPointsRegular, ppValue));
}
IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::get_OrientationProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::VirtualizingStackPanel_Orientation, ppValue));
}

// Attached properties.
_Check_return_ HRESULT DirectUI::VirtualizingStackPanelFactory::GetVirtualizationModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::VirtualizationMode* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::DependencyObject*>(pElement), KnownPropertyIndex::VirtualizingStackPanel_VirtualizationMode, pValue));
}

_Check_return_ HRESULT DirectUI::VirtualizingStackPanelFactory::SetVirtualizationModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::VirtualizationMode value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::DependencyObject*>(pElement), KnownPropertyIndex::VirtualizingStackPanel_VirtualizationMode, value));
}


IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::get_VirtualizationModeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::VirtualizingStackPanel_VirtualizationMode, ppValue));
}


IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::GetVirtualizationMode(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::VirtualizationMode* pValue)
{
    RRETURN(GetVirtualizationModeStatic(pElement, pValue));
}

IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::SetVirtualizationMode(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::VirtualizationMode value)
{
    RRETURN(SetVirtualizationModeStatic(pElement, value));
}
_Check_return_ HRESULT DirectUI::VirtualizingStackPanelFactory::GetIsVirtualizingStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pO, _Out_ BOOLEAN* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::DependencyObject*>(pO), KnownPropertyIndex::VirtualizingStackPanel_IsVirtualizing, pValue));
}

_Check_return_ HRESULT DirectUI::VirtualizingStackPanelFactory::SetIsVirtualizingStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pO, BOOLEAN value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::DependencyObject*>(pO), KnownPropertyIndex::VirtualizingStackPanel_IsVirtualizing, value));
}


IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::get_IsVirtualizingProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::VirtualizingStackPanel_IsVirtualizing, ppValue));
}


IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::GetIsVirtualizing(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pO, _Out_ BOOLEAN* pValue)
{
    RRETURN(GetIsVirtualizingStatic(pO, pValue));
}

IFACEMETHODIMP DirectUI::VirtualizingStackPanelFactory::SetIsVirtualizing(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pO, BOOLEAN value)
{
    RRETURN(SetIsVirtualizingStatic(pO, value));
}
_Check_return_ HRESULT DirectUI::VirtualizingStackPanelFactory::GetIsContainerGeneratedForInsertStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ BOOLEAN* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::VirtualizingStackPanel_IsContainerGeneratedForInsert, pValue));
}

_Check_return_ HRESULT DirectUI::VirtualizingStackPanelFactory::SetIsContainerGeneratedForInsertStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, BOOLEAN value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::VirtualizingStackPanel_IsContainerGeneratedForInsert, value));
}

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_VirtualizingStackPanel()
    {
        RRETURN(ctl::ActivationFactoryCreator<VirtualizingStackPanelFactory>::CreateActivationFactory());
    }
}
