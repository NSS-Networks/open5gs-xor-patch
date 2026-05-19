# open5gs-xor-patch
open5gs-authen-xor-patch
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/patches/0001-xor-aka-patch.patch
diff --git a/lib/crypt/meson.build b/lib/crypt/meson.build
index 2d42d13..28d8aac 100644
--- a/lib/crypt/meson.build
+++ b/lib/crypt/meson.build
@@ -38,6 +38,7 @@ libcrypt_sources = files('''
     ogs-sha2.c
     ogs-sha2-hmac.c
     milenage.c
+    xor.c
     snow-3g.c
     zuc.c
     kasumi.c
diff --git a/lib/crypt/ogs-crypt.h b/lib/crypt/ogs-crypt.h
index f0a8c69..b8652bd 100644
--- a/lib/crypt/ogs-crypt.h
+++ b/lib/crypt/ogs-crypt.h
@@ -32,6 +32,7 @@
 #include "crypt/ogs-aes-cmac.h"
 #include "crypt/milenage.h"
+#include "crypt/xor.h"
 #include "crypt/snow-3g.h"
 #include "crypt/zuc.h"
 #include "crypt/kasumi.h"
diff --git a/lib/dbi/subscription.c b/lib/dbi/subscription.c
index 94cf16c..24192cf 100644
--- a/lib/dbi/subscription.c
+++ b/lib/dbi/subscription.c
@@ -106,6 +106,10 @@ int ogs_dbi_auth_info(char *supi, ogs_dbi_auth_info_t *auth_info)
         } else if (!strcmp(key, OGS_SQN_STRING) &&
                 BSON_ITER_HOLDS_INT64(&inner_iter)) {
             auth_info->sqn = bson_iter_int64(&inner_iter);
+        } else if (!strcmp(key, "algo") &&
+                BSON_ITER_HOLDS_UTF8(&inner_iter)) {
+            utf8 = (char *)bson_iter_utf8(&inner_iter, &length);
+            ogs_cpystrn(auth_info->algo, utf8, sizeof(auth_info->algo));
         }
     }
diff --git a/lib/dbi/subscription.h b/lib/dbi/subscription.h
index 34324ac..c2124b9 100644
--- a/lib/dbi/subscription.h
+++ b/lib/dbi/subscription.h
@@ -36,6 +36,7 @@ typedef struct ogs_dbi_auth_info_s {
     uint8_t       amf[OGS_AMF_LEN];
     uint8_t       rand[OGS_RAND_LEN];
     uint64_t      sqn;
+    char          algo[16];
 } ogs_dbi_auth_info_t;
 int ogs_dbi_auth_info(char *supi, ogs_dbi_auth_info_t *auth_info);
diff --git a/src/udm/context.h b/src/udm/context.h
index 9a3cebd..7fe2284 100644
--- a/src/udm/context.h
+++ b/src/udm/context.h
@@ -65,6 +65,7 @@ struct udm_ue_s {
     uint8_t amf[OGS_AMF_LEN];
     uint8_t rand[OGS_RAND_LEN];
     uint8_t sqn[OGS_SQN_LEN];
+    char          algo[16];
     ogs_guami_t guami;
diff --git a/src/udm/nudr-handler.c b/src/udm/nudr-handler.c
index e11b932..7ff17e0 100644
--- a/src/udm/nudr-handler.c
+++ b/src/udm/nudr-handler.c
@@ -105,6 +105,8 @@ bool udm_nudr_dr_handle_subscription_authentication(
             }
             if (AuthenticationSubscription->authentication_method !=
+                    OpenAPI_auth_method_NONE &&
+                AuthenticationSubscription->authentication_method !=
                     OpenAPI_auth_method_5G_AKA) {
                 ogs_error("[%s] Not supported Auth Method [%d]",
                         udm_ue->suci,
@@ -209,8 +211,23 @@ bool udm_nudr_dr_handle_subscription_authentication(
 #endif
 #endif
-            milenage_generate(udm_ue->opc, udm_ue->amf, udm_ue->k, udm_ue->sqn,
-                    udm_ue->rand, autn, ik, ck, ak, xres, &xres_len);
+            if (AuthenticationSubscription->authentication_method ==
+                    OpenAPI_auth_method_NONE) {
+                /* XOR AKA - 3GPP TS 34.108 */
+                uint8_t ak[OGS_AK_LEN];
+                ogs_xor_f2345(udm_ue->k, udm_ue->rand,
+                        xres, ck, ik, ak);
+                xres_len = 8;
+                ogs_xor_f1(udm_ue->k, udm_ue->rand,
+                        udm_ue->sqn, udm_ue->amf, autn + 8);
+                for (int j = 0; j < 6; j++)
+                    autn[j] = udm_ue->sqn[j] ^ ak[j];
+                memcpy(autn + 6, udm_ue->amf, OGS_AMF_LEN);
+                ogs_log_hexdump(OGS_LOG_DEBUG, autn, OGS_AUTN_LEN);
+            } else {
+                milenage_generate(udm_ue->opc, udm_ue->amf, udm_ue->k, udm_ue->sqn,
+                        udm_ue->rand, autn, ik, ck, ak, xres, &xres_len);
+            }
             ogs_assert(udm_ue->serving_network_name);
