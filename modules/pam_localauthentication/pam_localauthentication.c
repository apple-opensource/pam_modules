/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 * Portions Copyright (c) 2001 PADL Software Pty Ltd. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Portions Copyright (c) 2000 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/******************************************************************
 * The purpose of this module is to provide a LocalAuthentication 
 * based authentication module for Mac OS X.
 ******************************************************************/

#include <CoreFoundation/CoreFoundation.h>
#include <coreauthd_spi.h>
#include <LocalAuthentication/LAPrivateDefines.h>

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT

#include <security/pam_modules.h>
#include <security/pam_appl.h>

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
    openpam_log(PAM_LOG_NOTICE, "pam_localauthentication: pam_sm_authenticate");

    int retval = PAM_AUTH_ERR;
    int tmpval = 0;
    CFDataRef *externalized_context = NULL;
    CFTypeRef context = NULL;
    CFErrorRef error = NULL;
    CFMutableDictionaryRef options = NULL;
    CFNumberRef key = NULL;
    CFNumberRef value = NULL;
    int tmp;

    /* get the externalized context */
    tmpval = pam_get_data(pamh, "token", (void *)&externalized_context);
    if (tmpval != PAM_SUCCESS) {
        openpam_log(PAM_LOG_ERROR, "error obtaining the token: %d", tmpval);
        retval = PAM_AUTHINFO_UNAVAIL;
        goto cleanup;
    }

    /* check that the externalized context is valid */
    if (!*externalized_context) {
        openpam_log(PAM_LOG_ERROR, "invalid token");
        retval = PAM_AUTHTOK_ERR;
        goto cleanup;
    }

    /* create a new LA context from the externalized context */
    context = LACreateNewContextWithACMContext(*externalized_context, &error);
    if (!context) {
        openpam_log(PAM_LOG_ERROR, "context creation failed: %ld", CFErrorGetCode(error));
        retval = PAM_AUTHTOK_ERR;
        goto cleanup;
    }

    /* prepare the options dictionary, aka rewrite @{ @(LAOptionNotInteractive) : @YES } without Foundation */
    tmp = kLAOptionNotInteractive;
    key = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &tmp);

    tmp = 1;
    value = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &tmp);

    options = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(options, key, value);

    /* evaluate policy */
    if (!LAEvaluatePolicy(context, kLAPolicyDeviceOwnerAuthenticationWithBiometrics, options, &error)) {
        openpam_log(PAM_LOG_ERROR, "policy evaluation failed: %ld", CFErrorGetCode(error));
        retval = PAM_AUTH_ERR;
        goto cleanup;
    }

    /* we passed the Touch ID authentication successfully */
    retval = PAM_SUCCESS;
    
cleanup:
    if (context) {
        CFRelease(context);
    }

    if (key) {
        CFRelease(key);
    }

    if (value) {
        CFRelease(value);
    }

    if (options) {
        CFRelease(options);
    }
    
    if (error) {
        CFRelease(error);
    }

    openpam_log(PAM_LOG_NOTICE, "pam_localauthentication: pam_sm_authenticate returned %d", retval);
    return retval;
}


PAM_EXTERN int
pam_sm_setcred(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
    return PAM_SUCCESS;
}


PAM_EXTERN int
pam_sm_acct_mgmt(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
    return PAM_SUCCESS;
}