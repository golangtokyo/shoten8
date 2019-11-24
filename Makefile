PHONY: lint fixlint build \
	help

# 引数がないときはusageを表示する
.DEFAULT_GOAL := help

lint: ## Execute textlint
	@$(shell npm bin)/textlint articles/*.re

fixlint: ## Fix textlint error
	@$(shell npm bin)/textlint --fix articles/*.re

build: ## Build PDF in Docker
	@./build-in-docker.sh

# 各コマンドについたコメントを表示する
help: ## Show usages
	@grep -E '^[a-zA-Z_-{\.}]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-15s\033[0m %s\n", $$1, $$2}'

