openssl ecparam -name prime256v1 -genkey -noout -out private.key
#openssl ecparam -name prime256v1 -genkey -noout -out private.key
openssl ec -in private.key -pubout -out public.key
#openssl ec -in private.key -pubout -out public.key
openssl ec -in public.key -pubout -outform DER -out raw_public.key
#openssl ec -in public.key -pubout -outform DER -out raw_public.key
openssl ec -in private.key -pubout -outform PEM -out keypair.pem
openssl x509 -req -in csr.pem -CA root.crt -CAkey root.key -CAcreateserial -out cert.pem -days 365
