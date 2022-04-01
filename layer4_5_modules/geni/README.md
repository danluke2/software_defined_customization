# GENI Customization Modules


Here we include modules used in the GENI networking environment. See experiment_scripts/geni for the tests performed with these modules.


* bulk: Insert/remove a tag every X bytes from the HTTP message.

    * arg: posit = 1000 (default)



* compress\_dns: Truncates the DNS request to include the transaction ID and FQDN only.


* end\_dns: Inserts a tag or series of tags to the end of a DNS request.

    * arg: tag\_count = 1 (default)

    * server side customization is optional b/c server will discard tag portion automatically

* front\_dns: Inserts a tag to the front of a DNS message

* middle\_dns: Inserts a tag after the DNS header and before the FQDN being queried
