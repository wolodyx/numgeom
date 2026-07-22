---
name: changelog
description: Generate a changelog between two Git tags or commits
---

# Changelog Generation Skill

Use this skill when the user asks to generate a changelog between two Git tags, commits, or versions.

## Instructions

1. **Determine the version range** from the user's request (e.g., `v0.1.0` to `v0.2.0`, or `v0.2.0..v0.2.7`).

2. **Retrieve the commit log** by running:
   ```
   git log --oneline <from>..<to>
   ```

3. **Retrieve full commit messages** (including bodies) for detailed descriptions:
   ```
   git log --oneline --format=""%h %ad %s"" --date=short <from>..<to>
   ```

4. **Categorize commits** by their conventional commit prefix:
   - `feat:` / `feature:` → ✨ Новые возможности (New Features)
   - `fix:` → 🐛 Исправления (Bug Fixes)
   - `build:` / `ci:` → 🔧 Сборка и CI (Build & CI)
   - `docs:` → 📖 Документация (Documentation)
   - `refactor:` → ♻️ Рефакторинг (Refactoring)
   - `test:` → 🧪 Тесты (Tests)
   - `perf:` → ⚡ Производительность (Performance)
   - `style:` → 🎨 Стиль кода (Code Style)
   - Other / mixed → 📝 Прочее (Other)

5. **Format the changelog** in Russian with the following structure:

   ```markdown
   # Changelog <from> → <to>

   ## ✨ Новые возможности
   - **Краткое описание** — подробное описание изменений. ([`<hash>`](<commit-url>))

   ## 🐛 Исправления
   - **Краткое описание** — подробное описание. ([`<hash>`](<commit-url>))

   ## 🔧 Сборка и CI
   - ...

   ## 📝 Прочее
   - ...

   ---
   **Всего коммитов:** N
   ```

6. **Commit URL format**: `https://github.com/numgeom/numgeom/commit/<hash>`

7. **Present the result** to the user using `attempt_completion` with a summary of the changelog.
