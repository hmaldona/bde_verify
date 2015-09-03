LIB_NAME=breguisvcmsg
SVC_NAME=breguisvc
SVC_ID=64730
SCHEMA_NAME=$(SVC_NAME).xsd
SRC_DIR=src
CFG_DIR=config
SVC_VERSION=1.12

default: $(LIB_NAME)

$(LIB_NAME):
	@echo creating library
	$(VERBOSE) plink -all -d source -64 breguisvcmsg.mk

clean: 
	@echo cleaning files
	$(VERBOSE) plink -all -d source -64 breguisvcmsg.mk clean

schema:
	rapi3tool --mode fetch --serviceInfo $(SVC_NAME):$(SVC_ID)-$(SVC_VERSION) --latest | tail +2 > $(CFG_DIR)/$(SCHEMA_NAME)

generate:
	bas_codegen.pl --mode msg --bidirectional --serviceInfo $(SVC_NAME):$(SVC_ID)-$(SVC_VERSION) --destination $(SRC_DIR) $(CFG_DIR)/$(SCHEMA_NAME)

test:
	basclient -l tcp://localhost:28588 -X $(CFG_DIR)/$(SCHEMA_NAME) -s $(SVC_ID)-$(SVC_VERSION) --trace --uuid 7034648 requests/getEntryValue.xml

test_local:
	basclient -l tcp://localhost:28588 -X $(CFG_DIR)/$(SCHEMA_NAME) -s $(SVC_ID)-$(SVC_VERSION) --trace --uuid 7034648 requests/searchByAccessor.xml

.PHONY: clean
.PHONY: schema
.PHONY: generate
.PHONY: test
.PHONY: test_local
